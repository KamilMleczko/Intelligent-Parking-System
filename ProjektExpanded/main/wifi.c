// wifi.c  — simplified SoftAP provisioning via HTTP + SPIFFS
// Exposes init_wifi(), which starts netif, mDNS, SoftAP, and web UI for provisioning.

#include "wifi.h"
#include <string.h>
#include <stdlib.h>
#include "esp_log.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_spiffs.h"
#include "esp_http_server.h"
#include "mdns.h"
#include "esp_wifi.h"
#include "cJSON.h"
#include "credentials.h"


// Max JSON body length
#define MAX_PROV_JSON_LEN  512

httpd_handle_t http_server;

static esp_err_t save_wifi_creds(const char *ssid, const char *pass) {
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open("wifi_creds", NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) return err;
    err  = nvs_set_str(nvs_handle, "ssid",     ssid);
    err |= nvs_set_str(nvs_handle, "pass",     pass);
    if (err == ESP_OK) err = nvs_commit(nvs_handle);
    nvs_close(nvs_handle);
    return err;
}

static bool load_wifi_creds(wifi_config_t *cfg) {
    nvs_handle_t nvs_handle;
    if (nvs_open("wifi_creds", NVS_READONLY, &nvs_handle) != ESP_OK) return false;

    size_t ssid_len = sizeof(cfg->sta.ssid);
    size_t pass_len = sizeof(cfg->sta.password);
    esp_err_t e1 = nvs_get_str(nvs_handle, "ssid", (char*)cfg->sta.ssid, &ssid_len);
    esp_err_t e2 = nvs_get_str(nvs_handle, "pass", (char*)cfg->sta.password, &pass_len);
    ESP_LOGI(TAG, "Loaded SSID='%s', PASS='%s'", cfg->sta.ssid, cfg->sta.password);
    nvs_close(nvs_handle);
    return (e1 == ESP_OK && e2 == ESP_OK && ssid_len>1);
}


static bool parse_prov_json(const char *body,
                            char *ssid_out, size_t ssid_len,
                            char *pass_out, size_t pass_len)
{
    cJSON *root = cJSON_Parse(body);
    if (!root) {
        ESP_LOGE(TAG, "cJSON_Parse failed");
        return false;
    }

    cJSON *ssid_item = cJSON_GetObjectItem(root, "ssid");
    cJSON *pass_item = cJSON_GetObjectItem(root, "passphrase");
    if (!cJSON_IsString(ssid_item) || !cJSON_IsString(pass_item)) {
        ESP_LOGE(TAG, "JSON missing string fields");
        cJSON_Delete(root);
        return false;
    }

    // copy safely
    strncpy(ssid_out, ssid_item->valuestring, ssid_len - 1);
    ssid_out[ssid_len - 1] = '\0';
    strncpy(pass_out, pass_item->valuestring, pass_len - 1);
    pass_out[pass_len - 1] = '\0';

    cJSON_Delete(root);
    return true;
}


// 1) Initialize network interfaces & event loop
static void init_netif(void) {
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();
    esp_netif_create_default_wifi_ap();
}

// 2) Mount SPIFFS (only once)
static void mount_spiffs(void) {
    esp_vfs_spiffs_conf_t conf = {
        .base_path              = "/spiffs",
        .partition_label        = NULL,
        .max_files              = 5,
        .format_if_mount_failed = true,
    };
    ESP_ERROR_CHECK(esp_vfs_spiffs_register(&conf));
}

// 3) Start SoftAP for provisioning
static void start_softap(void) {
    wifi_config_t ap_conf = { 0 };
    strncpy((char*)ap_conf.ap.ssid,     WIFI_AP_SSID, sizeof(ap_conf.ap.ssid)-1);
    strncpy((char*)ap_conf.ap.password, WIFI_AP_PASS, sizeof(ap_conf.ap.password)-1);
    ap_conf.ap.ssid_len = strlen(WIFI_AP_SSID);
    ap_conf.ap.max_connection = 4;
    ap_conf.ap.authmode = WIFI_AUTH_WPA2_PSK;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap_conf));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_LOGI(TAG, "SoftAP started: SSID=%s, PASS=%s", WIFI_AP_SSID, WIFI_AP_PASS);
}

// 4) Serve static files from SPIFFS/www
static esp_err_t static_get_handler(httpd_req_t *req) {
  const char *base = "/spiffs/www"; // 11 Bytes
  const char *uri  = req->uri; // 512 Bytes
  char filepath[524]; // 512 + 11 + 1 = 524 Bytes
  ESP_LOGI(TAG, "GET %s", uri);
  ESP_LOGI(TAG, "SERVING FILE: %s", filepath);
  
    if (strcmp(uri, "/") == 0) {
        snprintf(filepath, sizeof(filepath), "%s/index.html", base);
    } else {
        snprintf(filepath, sizeof(filepath), "%s%s", base, uri);
    }

    FILE *f = fopen(filepath, "r");
    if (!f) {
        httpd_resp_send_404(req);
        return ESP_FAIL;
    }
    char buf[256]; size_t r;
    do {
        r = fread(buf, 1, sizeof(buf), f);
        if (r > 0) httpd_resp_send_chunk(req, buf, r);
    } while (r == sizeof(buf));
    fclose(f);
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

static esp_err_t provision_post_handler(httpd_req_t *req)
{
    int len = req->content_len;
    if (len <= 0 || len > MAX_PROV_JSON_LEN) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Bad length");
        return ESP_FAIL;
    }

    char *body = malloc(len+1);
    if (!body) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "OOM");
        return ESP_FAIL;
    }
    int r = httpd_req_recv(req, body, len);
    if (r <= 0) {
        free(body);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Receive failed");
        return ESP_FAIL;
    }
    body[len] = '\0';

    char ssid[64], pass[64];
    if (!parse_prov_json(body, ssid, sizeof(ssid), pass, sizeof(pass))) {
        free(body);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON");
        return ESP_FAIL;
    }
    free(body);

    ESP_LOGI(TAG, "Provisioning to SSID=%s", ssid);
    // **Save** into flash
    if (save_wifi_creds(ssid, pass) != ESP_OK) {
        ESP_LOGW(TAG, "Failed to save creds");
    }
    ESP_LOGI(TAG, "Succesfully saved WiFi credentials!");

    httpd_stop(http_server);
    ESP_ERROR_CHECK( esp_wifi_stop() );

    // **Switch to STA** with new creds
    wifi_config_t sta_cfg = { 0 };
    strncpy((char*)sta_cfg.sta.ssid,     ssid, sizeof(sta_cfg.sta.ssid)-1);
    strncpy((char*)sta_cfg.sta.password, pass, sizeof(sta_cfg.sta.password)-1);
    ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK( esp_wifi_set_config(WIFI_IF_STA, &sta_cfg) );
    ESP_ERROR_CHECK( esp_wifi_start() );
    ESP_ERROR_CHECK( esp_wifi_connect() );

    // reply
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, "{\"status\":\"ok\"}");
    return ESP_OK;
}

// 6) Start HTTPD and register GET/POST handlers
static void start_http_server(void) {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    ESP_ERROR_CHECK(httpd_start(&http_server, &config));

    httpd_uri_t uri_root = {
        .uri      = "/",
        .method   = HTTP_GET,
        .handler  = static_get_handler,
        .user_ctx = NULL
    };
    ESP_ERROR_CHECK(httpd_register_uri_handler(http_server, &uri_root));

    httpd_uri_t uri_wild = {
        .uri      = "/*",
        .method   = HTTP_GET,
        .handler  = static_get_handler,
        .user_ctx = NULL
    };
    ESP_ERROR_CHECK(httpd_register_uri_handler(http_server, &uri_wild));

    httpd_uri_t post_uri = {
        .uri      = "/prov-config",
        .method   = HTTP_POST,
        .handler  = provision_post_handler,
        .user_ctx = NULL
    };
    ESP_ERROR_CHECK(httpd_register_uri_handler(http_server, &post_uri));
}

// 7) Initialize mDNS
static void init_mdns_service(void) {
    ESP_ERROR_CHECK(mdns_init());
    mdns_hostname_set("parking");
    mdns_instance_name_set("ParkingProvisioner");
    mdns_service_add(NULL, "_http", "_tcp", 80, NULL, 0);
}

// Exposed API: initialize everything for provisioning
void init_wifi(void) {
    esp_log_level_set("wifi", ESP_LOG_DEBUG);
    esp_log_level_set("esp_http_server", ESP_LOG_DEBUG);
    esp_log_level_set("httpd_uri", ESP_LOG_DEBUG);
    esp_log_level_set("httpd_txrx", ESP_LOG_DEBUG);
    esp_log_level_set("httpd", ESP_LOG_DEBUG);
    init_netif();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    wifi_config_t saved = { 0 };
    if (load_wifi_creds(&saved)) {
        ESP_LOGI(TAG, "Found saved SSID='%s', connecting…", saved.sta.ssid);
        ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );
        ESP_ERROR_CHECK( esp_wifi_set_config(WIFI_IF_STA, &saved) );
        ESP_ERROR_CHECK( esp_wifi_start() );
        ESP_ERROR_CHECK( esp_wifi_connect() );
        return;
    }

    mount_spiffs();
    start_softap();
    init_mdns_service();
    start_http_server();
}

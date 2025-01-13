package parking.api

import com.google.auth.oauth2.GoogleCredentials
import com.google.cloud.firestore.Firestore
import com.google.cloud.firestore.FirestoreOptions
import java.io.FileInputStream

object FirestoreClient {
    private var firestoreInstance: Firestore? = null

    fun getFirestore(): Firestore {
        if (firestoreInstance == null) {
            synchronized(this) {
                if (firestoreInstance == null) {
                    // Initialize Firestore with service account
                    val serviceAccount = FileInputStream("config/ServiceAccount.json")
                    val options = FirestoreOptions.newBuilder()
                        .setCredentials(GoogleCredentials.fromStream(serviceAccount))
                        .build()
                    firestoreInstance = options.service
                }
            }
        }
        return firestoreInstance!!
    }
}

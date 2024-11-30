import org.jetbrains.kotlin.gradle.dsl.jvm.JvmTargetValidationMode
import org.jetbrains.kotlin.gradle.tasks.KotlinCompile

plugins {
    kotlin("jvm") version "2.0.0"
    kotlin("plugin.serialization") version "2.0.0"
    `java-library`
    application
}

application {
    mainClass = "parking.MainKt"
}

group = "parking"
version = "1.0"

repositories {
    mavenCentral()
}
dependencies {
    api("org.eclipse.paho:org.eclipse.paho.client.mqttv3:1.2.5")
    api("org.jetbrains.kotlinx:kotlinx-serialization-json:1.6.0")
}

tasks.test {
    useJUnitPlatform()
}
tasks.withType<KotlinCompile> {
    compilerOptions{
        jvmTargetValidationMode.set(JvmTargetValidationMode.WARNING)
    }
}

tasks.withType<Jar>{
    manifest{
        attributes["Main-Class"] = "parking.MainKt"
    }
    duplicatesStrategy = DuplicatesStrategy.EXCLUDE

}


package parking.mqtt

import kotlin.reflect.KClass
import kotlin.reflect.full.primaryConstructor

fun <T : Any> deserialize(raw: String, klass: KClass<T>): Result<T> {
    return try {
        // Split the raw string into key-value pairs
        val keyValuePairs = raw.lines()
            .filter { it.isNotBlank() } // Ignore blank lines
            .associate { line ->
                val (key, value) = line.split(":", limit = 2)
                key.trim() to value.trim()
            }

        // Get the primary constructor and map its parameters
        val constructor = klass.primaryConstructor
            ?: throw IllegalArgumentException("Class ${klass.simpleName} must have a primary constructor.")

        // Map arguments to constructor parameters
        val args = constructor.parameters.associateWith { param ->
            val value = keyValuePairs[param.name]
            if (value != null) castValue(value.trim(), param.type.classifier as? KClass<*>)
            else if (!param.isOptional) throw IllegalArgumentException("Missing value for ${param.name}")
            else null
        }

        // Create an instance of the data class
        val instance = constructor.callBy(args)
        Result.success(instance)
    } catch (e: Exception) {
        Result.failure(e)
    }
}

fun castValue(value: String, targetType: KClass<*>?): Any? {
    return when (targetType) {
        String::class -> value
        Int::class -> value.toIntOrNull()
        Long::class -> value.toLongOrNull()
        Double::class -> value.toDoubleOrNull()
        Float::class -> value.toFloatOrNull()
        Boolean::class -> value.toBooleanStrictOrNull()
        else -> throw IllegalArgumentException("Unsupported type: $targetType")
    }
}

fun String.toBooleanStrictOrNull(): Boolean? {
    return when (this.lowercase()) {
        "true" -> true
        "false" -> false
        else -> null
    }
}

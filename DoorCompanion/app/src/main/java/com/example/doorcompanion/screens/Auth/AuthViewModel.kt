package com.example.doorcompanion.screens.Auth

import androidx.lifecycle.ViewModel
import com.google.firebase.auth.FirebaseAuth
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.asStateFlow

sealed class AuthState {
    data object Authenticated : AuthState()
    data object Loading : AuthState()
    data object Unauthenticated : AuthState()
    data class Error(val message: String) : AuthState()
}

sealed class SignUpState {
    data object UnAttempted : SignUpState()
    data object Success : SignUpState()
    data object Loading : SignUpState()
    data class Error(val message: String) : SignUpState()
}

class AuthViewModel : ViewModel() {

    private val auth = FirebaseAuth.getInstance()

    private val _authState = MutableStateFlow<AuthState>(AuthState.Unauthenticated)
    private val _signupState = MutableStateFlow<SignUpState>(SignUpState.UnAttempted)
    val authState = _authState.asStateFlow()
    val signupState = _signupState.asStateFlow()

    fun login(email: String, password: String, successCallback: () -> Unit = {}) {
        _authState.value = AuthState.Loading
        auth.signInWithEmailAndPassword(email, password)
            .addOnCompleteListener { task ->
                if (task.isSuccessful) {
                    _authState.value = AuthState.Authenticated
                    successCallback()
                } else {
                    _authState.value = AuthState.Error(
                        task.exception?.message ?: "Something went wrong while attempting to log in"
                    )
                }
            }
    }

    fun signup(email: String, password: String, successCallback: () -> Unit = {}) {
        _signupState.value = SignUpState.Loading
        auth.createUserWithEmailAndPassword(email, password)
            .addOnCompleteListener { task ->
                if (task.isSuccessful) {
                    _signupState.value = SignUpState.Success
                    successCallback()
                } else
                    _signupState.value = SignUpState.Error(
                        task.exception?.message ?: "Something went wrong when attempting sign up"
                    )
            }
    }

    fun signOut() {
        auth.signOut()
        _authState.value = AuthState.Unauthenticated
    }

}
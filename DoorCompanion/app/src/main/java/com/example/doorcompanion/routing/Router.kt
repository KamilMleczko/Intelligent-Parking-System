package com.example.doorcompanion.routing

import Screen
import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.SharingStarted
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.map
import kotlinx.coroutines.flow.stateIn

class Router : ViewModel() {
    private val _screenStack = MutableStateFlow<List<Screen>>(listOf(Screen.BleScan))

    val currentScreen: StateFlow<Screen?> = _screenStack
        .map { stack ->
            if (stack.isEmpty()) null else stack.last()
        }
        .stateIn(
            scope = viewModelScope,
            // Starts upon initialization, before a subscription happens
            started = SharingStarted.Eagerly,
            initialValue = Screen.BleScan
        )

    val pageNumber = _screenStack
        .map { stack ->
            stack.size
        }
        .stateIn(
            scope = viewModelScope,
            started = SharingStarted.Eagerly,
            initialValue = 0
        )

    fun navigateTo(screen: Screen) {
        _screenStack.value += screen
    }

    fun goBack() {
        if (_screenStack.value.size > 1) {
            _screenStack.value = _screenStack.value.dropLast(1)
        }
    }


}
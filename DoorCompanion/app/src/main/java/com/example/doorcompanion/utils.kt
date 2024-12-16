package com.example.doorcompanion

import android.content.Context
import android.widget.Toast

fun toastNotify(context: Context, text: String){
    val toast = Toast(context)
    toast.setText(text)
    toast.duration = Toast.LENGTH_SHORT;
    toast.show();
}
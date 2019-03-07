package com.ymz.terminal

import android.util.Log
import java.io.FileDescriptor
import java.lang.reflect.Field

/**
 *Create by YMZ
 *2019-3-7
 */

val LOG_TAG = "FileDescriptorExtend"

fun FileDescriptor.getFd(): Int {
    val fd: Int
    val fdField: Field = try {
        FileDescriptor::class.java.getDeclaredField("descriptor")
    } catch (e: NoSuchFieldException) {
        FileDescriptor::class.java.getDeclaredField("fd")
    }
    fdField.isAccessible = true
    fd = fdField.getInt(this)
    return fd
}

fun Int.toFileDescriptor(): FileDescriptor {
    val result = FileDescriptor()
    try {
        val descriptorField: Field = try {
            FileDescriptor::class.java.getDeclaredField("descriptor")
        } catch (e: NoSuchFieldException) {
            // For desktop java:
            FileDescriptor::class.java.getDeclaredField("fd")
        }
        descriptorField.isAccessible = true
        descriptorField.set(result, this)
    } catch (e: NoSuchFieldException) {
        Log.wtf(LOG_TAG, "Error accessing FileDescriptor#descriptor private field", e)
        System.exit(1)
    } catch (e: IllegalAccessException) {
        Log.wtf(LOG_TAG, "Error accessing FileDescriptor#descriptor private field", e)
        System.exit(1)
    } catch (e: IllegalArgumentException) {
        Log.wtf(LOG_TAG, "Error accessing FileDescriptor#descriptor private field", e)
        System.exit(1)
    }
    return result
}
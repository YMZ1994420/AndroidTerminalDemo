package com.ymz.androidterminaldemo

import android.support.v7.app.AppCompatActivity
import android.os.Bundle
import android.os.Handler
import com.ymz.terminal.Terminal
import kotlinx.android.synthetic.main.activity_main.*

class MainActivity : AppCompatActivity() {

    val handler = Handler()

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        // Example of a call to a native method

        btnTest.setOnClickListener {
            Terminal{
                handler.post {
                    sample_text.append(it)
                }
            }.apply {
                executeCmd("ls")
            }
        }
    }

    /**
     * A native method that is implemented by the 'native-lib' native library,
     * which is packaged with this application.
     */
    external fun stringFromJNI(): String

    companion object {

        // Used to load the 'native-lib' library on application startup.
        init {
            System.loadLibrary("native-lib")
        }
    }
}

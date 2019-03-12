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
}

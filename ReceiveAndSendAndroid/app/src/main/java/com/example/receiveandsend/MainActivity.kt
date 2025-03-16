package com.example.receiveandsend

import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle
import android.widget.Button
import android.widget.EditText
import android.widget.Toast
import okhttp3.*
import java.io.IOException

class MainActivity : AppCompatActivity() {

    private val client = OkHttpClient()
    private val esp32BaseUrl = "http://192.168.15.52"

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        val btnSend = findViewById<Button>(R.id.btnSendCode)
        val etSendIndex = findViewById<EditText>(R.id.etSendIndex)
        val btnRegister = findViewById<Button>(R.id.btnRegisterCode)
        val etRegisterIndex = findViewById<EditText>(R.id.etRegisterIndex)

        btnSend.setOnClickListener {
            val indexText = etSendIndex.text.toString()
            if (indexText.isNotEmpty()) {
                sendIRCode(indexText.toInt())
            } else {
                Toast.makeText(this, "Insira o numero do comando (0-9)", Toast.LENGTH_SHORT).show()
            }
        }

        btnRegister.setOnClickListener {
            val indexText = etRegisterIndex.text.toString()
            if (indexText.isNotEmpty()) {
                registerIRCode(indexText.toInt())
            } else {
                Toast.makeText(this, "Insira um numero para registrar (0-9)", Toast.LENGTH_SHORT).show()
            }
        }
    }

    private fun sendIRCode(index: Int) {
        val url = "$esp32BaseUrl/send?index=$index"
        val request = Request.Builder()
            .url(url)
            .build()

        client.newCall(request).enqueue(object: Callback {
            override fun onFailure(call: Call, e: IOException) {
                runOnUiThread {
                    Toast.makeText(this@MainActivity, "Falha na requisição: ${e.message}", Toast.LENGTH_SHORT).show()
                }
            }

            override fun onResponse(call: Call, response: Response) {
                runOnUiThread {
                    if (response.isSuccessful) {
                        Toast.makeText(this@MainActivity, "Comando enviado com sucesso!", Toast.LENGTH_SHORT).show()
                    } else {
                        Toast.makeText(this@MainActivity, "Erro: ${response.code}", Toast.LENGTH_SHORT).show()
                    }
                }
            }
        })
    }

    private fun registerIRCode(index: Int) {
        val url = "$esp32BaseUrl/register?index=$index"
        val request = Request.Builder()
            .url(url)
            .build()

        client.newCall(request).enqueue(object: Callback {
            override fun onFailure(call: Call, e: IOException) {
                runOnUiThread {
                    Toast.makeText(this@MainActivity, "Erro ao registrar comando: ${e.message}", Toast.LENGTH_SHORT).show()
                }
            }

            override fun onResponse(call: Call, response: Response) {
                runOnUiThread {
                    if (response.isSuccessful) {
                        Toast.makeText(this@MainActivity, "Pressione o botão do controle para registar", Toast.LENGTH_LONG).show()
                    } else {
                        Toast.makeText(this@MainActivity, "Erro: ${response.code}", Toast.LENGTH_SHORT).show()
                    }
                }
            }
        })
    }
}

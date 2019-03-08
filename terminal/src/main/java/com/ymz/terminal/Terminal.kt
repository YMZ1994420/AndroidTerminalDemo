package com.ymz.terminal

import android.util.Log
import java.io.FileDescriptor
import java.io.FileInputStream
import java.io.FileOutputStream
import java.util.concurrent.ConcurrentHashMap

/**
 *Create by YMZ
 *2019-3-7
 */
/**
 * @property onShellOut  回调的参数是经过处理的从pts中读取的数据。
 */
//todo 需要提供同步的输入和异步的输入
class Terminal(private val onShellOut: (String) -> Unit) {

    private var ptmFd: FileDescriptor
    private var pid: Int
    private val expectMap = ConcurrentHashMap<String, () -> Unit>()

    init {
        val pidOut = intArrayOf(0)
        ptmFd = Jni.createSubprocess("/system/bin/sh", "", listOf("-i").toTypedArray(), null, pidOut).toFileDescriptor()
        pid = pidOut[0]
        Thread({
            val buffer = ByteArray(4096)
            try {
                FileInputStream(ptmFd).let {
                    while (true) {
                        val read = it.read(buffer)
                        if (read == -1) {
                            //终端关闭
                            break
                        } else {
                            val data = String(buffer.copyOfRange(0, read))
                            onShellOut.invoke(data)
                            expectMap.forEach { keyValue ->
                                if (data.contains(keyValue.key, true)) {
                                    keyValue.value.invoke()
                                }
                            }
                        }
                    }
                }
            } catch (e: Throwable) {
                debug("终端已关闭,错误原因:${e.message}")
                e.printStackTrace()
            }
        }, "TerminalProducter($pid)").start()
    }


    fun stopShell() {
        Jni.close(ptmFd.getFd())
    }

    fun executeCmd(cmd: String) {
        FileOutputStream(ptmFd).let {
            it.write(cmd.toByteArray())
            if (!cmd.endsWith("\n")) {
                it.write("\n".toByteArray())
            }
        }
    }

    fun interrupt() {
        FileOutputStream(ptmFd).let {
            it.write(byteArrayOf(3))
        }
    }

    /**
     * 当终端输出的最后数据与期待的数据匹配时执行action
     */
    fun registExepectAction(expect: String, action: () -> Unit) {
        expectMap[expect] = action
    }

    fun unregistExepectAction(expect: String) {
        expectMap.remove(expect)
    }

    private inline fun <reified T> T.debug(log: String) {
        Log.d("AndroidTerminal", log)
    }
}
//
// Created by YMZ on 2019-3-7.
//
#include "terminal.h"
#include "android/log.h"
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <wait.h>

#define LOG_TAG "androidterminal"
#define LOG(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

static int throw_runtime_exception(JNIEnv *env, char const *message) {
    jclass exClass = env->FindClass("java/lang/RuntimeException");
    env->ThrowNew(exClass, message);
    return -1;
}

JNIEXPORT jint JNICALL Java_com_ymz_terminal_Jni_createSubprocess
        (JNIEnv *jniEnv, jclass jclass, jstring cmd, jstring cwd, jobjectArray args, jobjectArray enVars,
         jintArray subprocessId) {
    //打开ptmx获得独立的fd
    auto ptmx = open("/dev/ptmx", O_RDWR | O_CLOEXEC);
    if (ptmx < 0) return throw_runtime_exception(jniEnv, "打开ptmx文件失败");
    LOG("打开ptmx成功");
    //通过ptmx的fd获得pts
    char devname[64];
    if (grantpt(ptmx) || unlockpt(ptmx) || ptsname_r(ptmx, devname, sizeof(devname))) {
        return throw_runtime_exception(jniEnv, "获取pts文件失败");
    }
    LOG("打开fd成功");
    //fork子进程
    jboolean isCopy;
    auto pid = fork();
    if (pid < 0) {
        return throw_runtime_exception(jniEnv, "创建子进程失败");
    } else if (pid > 0) {
        LOG("子进程创建成功");
        //返回ptmx与子进程的id
        int *pProcId = (int *) jniEnv->GetPrimitiveArrayCritical(subprocessId, &isCopy);
        if (pProcId) {
            *pProcId = pid;
            jniEnv->ReleasePrimitiveArrayCritical(subprocessId, pProcId, 0);
        }
        return ptmx;
    } else {
        close(ptmx);
        // Clear signals which the Android java process may have blocked:
        sigset_t signals_to_unblock;
        sigfillset(&signals_to_unblock);
        sigprocmask(SIG_UNBLOCK, &signals_to_unblock, 0);
        //子进程创建新的session
        setsid();
        LOG("新建会话成功");
        //子进程的标准输入输出设为pts
        int pts = open(devname, O_RDWR);
        if (pts < 0) exit(-1);
        dup2(pts, 0);
        dup2(pts, 1);
        dup2(pts, 2);
        LOG("绑定输入输出成功");
//        auto bash = jniEnv->GetStringUTFChars(cmd, &isCopy);
//        auto length = jniEnv->GetArrayLength(args);
//        char *argsC[length];
//        for (int i = 0; i < length; i++) {
//            strcpy(argsC[i], (const char *) (jniEnv->GetObjectArrayElement(args, length)));
//        }
        //一般而言，bash为"system/bin/sh",args为"-i"
        LOG("准备创建BASH");
        char * argsC[2];
        argsC[0] = const_cast<char *>("-i");
        argsC[1] = nullptr;
        execvp("/system/bin/sh", argsC);
        char* error_message;
        if (asprintf(&error_message, "创建的命令为: exec(\"%s\")", cmd) == -1) error_message = "exec()";
        perror(error_message);
        _exit(1);
        LOG("BASH创建失败");
    }
}

JNIEXPORT void JNICALL Java_com_ymz_terminal_Jni_close
        (JNIEnv *jniEnv, jclass jclass, jint fileDescriptor) {
    close(fileDescriptor);
}

JNIEXPORT jint JNICALL Java_com_ymz_terminal_Jni_waitFor
        (JNIEnv *jniEnv, jclass jclass, jint pid) {
    int status;
    waitpid(pid, &status, 0);
    if (WIFEXITED(status)) {
        return WEXITSTATUS(status);
    } else if (WIFSIGNALED(status)) {
        return -WTERMSIG(status);
    } else {
        // Should never happen - waitpid(2) says "One of the first three macros will evaluate to a non-zero (true) value".
        return 0;
    }
}

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

static jint
createSubProcess(JNIEnv *jniEnv, jclass jclass, const char file[], const char cwd[], char *args[], char *envars[],
                 pid_t &subprocessID) {
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
    auto pid = fork();
    if (pid < 0) {
        return throw_runtime_exception(jniEnv, "创建子进程失败");
    } else if (pid > 0) {
        LOG("子进程创建成功");
        //返回ptmx与子进程的id
        LOG("取得的processId: %d", pid);
        subprocessID = pid;
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
        LOG("执行EXEC,参数：文件： %s，参数: %s", file, args[0]);
        execvp(file, args);
        char *error_message;
        if (asprintf(&error_message, "exec(\"%s\")", file) == -1) error_message = "exec()";
        perror(error_message);
        _exit(1);
    }
}

//todo 从javaObject中获得正确的C类型的方法.目前填写的参数基本没用上
JNIEXPORT jint JNICALL Java_com_ymz_terminal_Jni_createSubprocess
        (JNIEnv *jniEnv, jclass jclass, jstring file, jstring cwd, jobjectArray args, jobjectArray enVars,
         jintArray subprocessId) {
    //转换JNI基础类型及String为C++类型,基础类型最好直接使用，对象类型通过jniEnv进行转换
    jboolean isCopy; //告知生成的对象是产生的内存拷贝还是与原对象的指针。JVM尽可能返回一个直接的指针，否则返回一个拷贝。不要对直接指针进行修改，如果不需要修改可以直接传入空指针。
    auto c_file = jniEnv->GetStringUTFChars(file, &isCopy);
    auto c_cwd = jniEnv->GetStringUTFChars(cwd, nullptr);
    pid_t pid;
    //转换JNI的object数组需要遍历数组并复制成员。

    //使用C函数执行代码。
    createSubProcess(jniEnv, jclass, c_file, c_cwd, nullptr, nullptr, pid);
    //要想对java提供out-in类型的数组赋值，需要使用获取引用的critical方法。注意critical方法的get/release之间不允许调用任何jniEnv的方法。
//    auto c_subProcess = static_cast<jint *>(jniEnv->GetPrimitiveArrayCritical(subprocessId, nullptr));
//    if (c_subProcess) {
//        c_subProcess[0] = pid;
//    }
//    jniEnv->ReleasePrimitiveArrayCritical(subprocessId,c_subProcess,JNI_ABORT);
    //使用之后请释放生成的copy或引用以便JVM进行回收
//    jniEnv->ReleaseStringUTFChars(file, c_file);//子进程可能此时还未完成，如果是拷贝，不应该这个时候进行销毁
//    jniEnv->ReleaseStringUTFChars(cwd, c_cwd);//子进程可能此时还未完成，如果是拷贝，不应该这个时候进行销毁
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

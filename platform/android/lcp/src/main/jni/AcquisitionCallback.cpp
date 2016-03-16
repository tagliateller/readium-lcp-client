//
// Created by clebeaupin on 15/03/16.
//

#include "AcquisitionCallback.h"
#include "Util.h"

namespace lcp {
    AcquisitionCallback::AcquisitionCallback(jobject jAcquisitionCallback) {
        JNIEnv * env = getJNIEnv();
        this->jAcquisitionCallback = env->NewGlobalRef(jAcquisitionCallback);
        jclass jAcquisitionCallbackClass = env->GetObjectClass(this->jAcquisitionCallback);
        this->jOnAcquisitionStartedMethodId = env->GetMethodID(
                jAcquisitionCallbackClass, "onAcquisitionStarted", "()V");
        this->jOnAcquisitionEndedMethodId = env->GetMethodID(
                jAcquisitionCallbackClass, "onAcquisitionEnded", "()V");
        this->jOnAcquisitionProgressedMethodId = env->GetMethodID(
                jAcquisitionCallbackClass, "onAcquisitionProgressed", "(F)V");
    }

    AcquisitionCallback::~AcquisitionCallback() {
        JNIEnv * env = getJNIEnv();
        env->DeleteGlobalRef(this->jAcquisitionCallback);
    }

    void AcquisitionCallback::OnAcquisitionStarted(IAcquisition * acquisition) {
        JNIEnv * env = getJNIEnv();
        env->CallVoidMethod(
                this->jAcquisitionCallback, jOnAcquisitionStartedMethodId);
    }

    void AcquisitionCallback::OnAcquisitionProgressed(IAcquisition * acquisition, float progress) {
        JNIEnv * env = getJNIEnv();
        env->CallVoidMethod(
                this->jAcquisitionCallback, jOnAcquisitionProgressedMethodId, (jfloat) progress);
    }

    void AcquisitionCallback::OnAcquisitionCanceled(IAcquisition * acquisition) {
    }

    void AcquisitionCallback::OnAcquisitionEnded(IAcquisition * acquisition, Status result) {
        JNIEnv * env = getJNIEnv();
        env->CallVoidMethod(
                this->jAcquisitionCallback, jOnAcquisitionEndedMethodId);
    }
}
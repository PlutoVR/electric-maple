#pragma once

#include <android/log.h>

#define ALOGV(...) __android_log_print(ANDROID_LOG_VERBOSE, "ElectricMaple", __VA_ARGS__)
#define ALOGD(...) __android_log_print(ANDROID_LOG_DEBUG, "ElectricMaple", __VA_ARGS__)
#define ALOGI(...) __android_log_print(ANDROID_LOG_INFO, "ElectricMaple", __VA_ARGS__)
#define ALOGW(...) __android_log_print(ANDROID_LOG_WARN, "ElectricMaple", __VA_ARGS__)
#define ALOGE(...) __android_log_print(ANDROID_LOG_ERROR, "ElectricMaple", __VA_ARGS__)

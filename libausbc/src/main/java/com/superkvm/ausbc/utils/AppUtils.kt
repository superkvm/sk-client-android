
package com.superkvm.ausbc.utils

import android.annotation.SuppressLint
import android.app.ActivityManager
import android.app.AlarmManager
import android.app.PendingIntent
import android.content.Context
import android.content.Intent
import android.content.pm.PackageInfo
import android.content.pm.PackageManager
import android.os.Process



object AppUtils {

    @SuppressLint("UnspecifiedImmutableFlag")
    fun restartApp(ctx: Context?) {
        ctx ?: return
        val pckgManager: PackageManager = ctx.applicationContext.packageManager
        val intent: Intent? = pckgManager.getLaunchIntentForPackage(ctx.packageName)
        intent?.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK)
        val pendingIntent: PendingIntent = PendingIntent.getActivity(
            ctx.applicationContext, 0, intent, PendingIntent.FLAG_ONE_SHOT
        )
        val manager: AlarmManager = ctx.applicationContext.getSystemService(Context.ALARM_SERVICE) as AlarmManager
        manager.set(AlarmManager.RTC, System.currentTimeMillis() + 1000, pendingIntent)
    }

    fun releaseAppResource() {
        Process.killProcess(Process.myPid())
        System.exit(0)
    }

    fun removeAllActivity() {
        ActivityStackUtils.popAllActivity()
    }

    fun getAppName(ctx: Context): String? {
        val packageManager: PackageManager = ctx.packageManager
        try {
            val packageInfo: PackageInfo = packageManager.getPackageInfo(ctx.packageName, 0)
            val labelRes: Int = packageInfo.applicationInfo.labelRes
            return ctx.getString(labelRes)
        } catch (e: PackageManager.NameNotFoundException) {
            e.printStackTrace()
        }
        return null
    }

    
    fun isServiceRunning(context: Context, className: String): Boolean {
        var isRunning = false
        val activityManager = context.getSystemService(Context.ACTIVITY_SERVICE) as ActivityManager
        val services = activityManager.getRunningServices(Int.MAX_VALUE)
        if (services != null && services.size > 0) {
            for (service in services) {
                if (className == service.service.className) {
                    isRunning = true
                    break
                }
            }
        }
        return isRunning
    }
}
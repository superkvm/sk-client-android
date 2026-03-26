
package com.superkvm.kvm

import android.Manifest.permission.*
import android.os.Bundle
import android.os.PowerManager
import android.util.Log
import androidx.appcompat.app.AppCompatActivity
import androidx.core.app.ActivityCompat
import androidx.core.content.PermissionChecker
import androidx.fragment.app.Fragment
import com.superkvm.ausbc.utils.ToastUtils
import android.view.WindowManager
import android.os.Build
import androidx.core.view.WindowCompat
import com.superkvm.ausbc.utils.Utils
import com.superkvm.kvm.databinding.ActivityMainBinding


class MainActivity : AppCompatActivity() {
    private var mWakeLock: PowerManager.WakeLock? = null
    private lateinit var viewBinding: ActivityMainBinding

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        val marker = "superkvm BUILD_TYPE_MARK activity=${this::class.java.simpleName}, type=${BuildConfig.BUILD_TYPE}"
        Log.e("superkvm", marker)
        System.err.println(marker)
        setStatusBar()
        viewBinding = ActivityMainBinding.inflate(layoutInflater)
        setContentView(viewBinding.root)
        replaceMainFragment(MainFragment())
    }

    override fun onStart() {
        super.onStart()
        mWakeLock = Utils.wakeLock(this)
    }

    override fun onStop() {
        super.onStop()
        mWakeLock?.apply {
            Utils.wakeUnLock(this)
        }
    }

    private fun replaceMainFragment(fragment: Fragment) {
        val hasCameraPermission = PermissionChecker.checkSelfPermission(this, CAMERA)
        val hasStoragePermission =
            PermissionChecker.checkSelfPermission(this, WRITE_EXTERNAL_STORAGE)
        if (hasCameraPermission != PermissionChecker.PERMISSION_GRANTED || hasStoragePermission != PermissionChecker.PERMISSION_GRANTED) {
            if (ActivityCompat.shouldShowRequestPermissionRationale(this, CAMERA)) {
                ToastUtils.show(R.string.permission_tip)
            }
            ActivityCompat.requestPermissions(
                this,
                arrayOf(CAMERA, WRITE_EXTERNAL_STORAGE, RECORD_AUDIO),
                REQUEST_CAMERA
            )
            return
        }
        val transaction = supportFragmentManager.beginTransaction()
        transaction.replace(R.id.fragment_container, fragment)
        transaction.commitAllowingStateLoss()
    }

    override fun onRequestPermissionsResult(
        requestCode: Int,
        permissions: Array<out String>,
        grantResults: IntArray
    ) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults)
        when (requestCode) {
            REQUEST_CAMERA -> {
                val hasCameraPermission = PermissionChecker.checkSelfPermission(this, CAMERA)
                if (hasCameraPermission == PermissionChecker.PERMISSION_DENIED) {
                    ToastUtils.show(R.string.permission_tip)
                    return
                }
                replaceMainFragment(MainFragment())
            }
            REQUEST_STORAGE -> {
                val hasCameraPermission =
                    PermissionChecker.checkSelfPermission(this, WRITE_EXTERNAL_STORAGE)
                if (hasCameraPermission == PermissionChecker.PERMISSION_DENIED) {
                    ToastUtils.show(R.string.permission_tip)
                    return
                }
                
            }
            else -> {
            }
        }
    }

    private fun setStatusBar() {
        
        WindowCompat.setDecorFitsSystemWindows(window, true)
        
        
        window.statusBarColor = resources.getColor(R.color.black, theme)
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
            window.navigationBarColor = resources.getColor(R.color.black, theme)
        }
        
        
        val windowInsetsController = WindowCompat.getInsetsController(window, window.decorView)
        windowInsetsController?.isAppearanceLightStatusBars = false
        
        
        window.setSoftInputMode(WindowManager.LayoutParams.SOFT_INPUT_ADJUST_RESIZE)
    }

    companion object {
        private const val REQUEST_CAMERA = 0
        private const val REQUEST_STORAGE = 1
    }
}

package com.superkvm.ausbc.utils.bus

import androidx.annotation.MainThread
import androidx.lifecycle.*
import java.util.concurrent.ConcurrentHashMap


object EventBus {
    private val mLiveDataMap = ConcurrentHashMap<String, LiveData<*>>()

    
    fun <T> with(key: String): BusLiveData<T> {
        var liveData = mLiveDataMap[key] as? BusLiveData<T>
        if (liveData == null) {
            liveData = BusLiveData<T>(key).apply {
                mLiveDataMap[key] = this
            }
        }
        return liveData
    }

    
    class BusLiveData<T>(private val busName: String): MutableLiveData<T>() {
        internal var mVersion = 0

        
        @MainThread
        fun sendMessage(message: T) {
            ++mVersion
            value = message
        }

        
        fun postMessage(message: T) {
            ++mVersion
            postValue(message)
        }

        override fun observe(owner: LifecycleOwner, observer: Observer<in T>) {
            
            
            owner.lifecycle.addObserver(object : LifecycleEventObserver {
                override fun onStateChanged(source: LifecycleOwner, event: Lifecycle.Event) {
                    if (event == Lifecycle.Event.ON_DESTROY) {
                        if (mLiveDataMap[busName]?.hasObservers() == false) {
                            mLiveDataMap.remove(busName)
                        }
                    }
                }
            })
            
            
            super.observe(owner, ProxyObserver(this, observer))
        }
    }

    
    internal class ProxyObserver<T>(
        private val liveData: BusLiveData<T>,
        private val observer: Observer<in T>
    ): Observer<T> {
        
        private var mLastVersion = liveData.mVersion

        
        
        override fun onChanged(data: T) {
            if (mLastVersion >= liveData.mVersion) {
                return
            }
            mLastVersion = liveData.mVersion
            observer.onChanged(data)
        }
    }
}
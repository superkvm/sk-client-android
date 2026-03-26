
package com.superkvm.ausbc.render.effect

import android.content.Context
import com.superkvm.ausbc.render.internal.AbstractFboRender


abstract class AbstractEffect(ctx: Context) : AbstractFboRender(ctx) {

    
    abstract fun getId(): Int

    
    abstract fun getClassifyId(): Int
}
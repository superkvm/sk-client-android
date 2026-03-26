
package com.superkvm.ausbc.render.effect

import android.content.Context
import com.superkvm.ausbc.R
import com.superkvm.ausbc.render.effect.bean.CameraEffect


class EffectBlackWhite(ctx: Context) : AbstractEffect(ctx) {

    override fun getId(): Int = ID

    override fun getClassifyId(): Int = CameraEffect.CLASSIFY_ID_FILTER

    override fun getVertexSourceId(): Int = R.raw.base_vertex

    override fun getFragmentSourceId(): Int = R.raw.effect_blackw_fragment

    companion object {
        const val ID = 100
    }
}
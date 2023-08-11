// automatically generated by the FlatBuffers compiler, do not modify

package org.luckypray.dexkit.schema

import com.google.flatbuffers.BaseVector
import com.google.flatbuffers.BooleanVector
import com.google.flatbuffers.ByteVector
import com.google.flatbuffers.Constants
import com.google.flatbuffers.DoubleVector
import com.google.flatbuffers.FlatBufferBuilder
import com.google.flatbuffers.FloatVector
import com.google.flatbuffers.LongVector
import com.google.flatbuffers.StringVector
import com.google.flatbuffers.Struct
import com.google.flatbuffers.Table
import com.google.flatbuffers.UnionVector
import java.nio.ByteBuffer
import java.nio.ByteOrder
import kotlin.math.sign

@Suppress("unused")
class AnnotationsMatcher : Table() {

    fun __init(_i: Int, _bb: ByteBuffer)  {
        __reset(_i, _bb)
    }
    fun __assign(_i: Int, _bb: ByteBuffer) : AnnotationsMatcher {
        __init(_i, _bb)
        return this
    }
    fun annotations(j: Int) : AnnotationMatcher? = annotations(AnnotationMatcher(), j)
    fun annotations(obj: AnnotationMatcher, j: Int) : AnnotationMatcher? {
        val o = __offset(4)
        return if (o != 0) {
            obj.__assign(__indirect(__vector(o) + j * 4), bb)
        } else {
            null
        }
    }
    val annotationsLength : Int
        get() {
            val o = __offset(4); return if (o != 0) __vector_len(o) else 0
        }
    val matchType : Byte
        get() {
            val o = __offset(6)
            return if(o != 0) bb.get(o + bb_pos) else 0
        }
    val annotaionCount : IntRange? get() = annotaionCount(IntRange())
    fun annotaionCount(obj: IntRange) : IntRange? {
        val o = __offset(8)
        return if (o != 0) {
            obj.__assign(__indirect(o + bb_pos), bb)
        } else {
            null
        }
    }
    companion object {
        fun validateVersion() = Constants.FLATBUFFERS_23_5_26()
        fun getRootAsAnnotationsMatcher(_bb: ByteBuffer): AnnotationsMatcher = getRootAsAnnotationsMatcher(_bb, AnnotationsMatcher())
        fun getRootAsAnnotationsMatcher(_bb: ByteBuffer, obj: AnnotationsMatcher): AnnotationsMatcher {
            _bb.order(ByteOrder.LITTLE_ENDIAN)
            return (obj.__assign(_bb.getInt(_bb.position()) + _bb.position(), _bb))
        }
        fun createAnnotationsMatcher(builder: FlatBufferBuilder, annotationsOffset: Int, matchType: Byte, annotaionCountOffset: Int) : Int {
            builder.startTable(3)
            addAnnotaionCount(builder, annotaionCountOffset)
            addAnnotations(builder, annotationsOffset)
            addMatchType(builder, matchType)
            return endAnnotationsMatcher(builder)
        }
        fun startAnnotationsMatcher(builder: FlatBufferBuilder) = builder.startTable(3)
        fun addAnnotations(builder: FlatBufferBuilder, annotations: Int) = builder.addOffset(0, annotations, 0)
        fun createAnnotationsVector(builder: FlatBufferBuilder, data: IntArray) : Int {
            builder.startVector(4, data.size, 4)
            for (i in data.size - 1 downTo 0) {
                builder.addOffset(data[i])
            }
            return builder.endVector()
        }
        fun startAnnotationsVector(builder: FlatBufferBuilder, numElems: Int) = builder.startVector(4, numElems, 4)
        fun addMatchType(builder: FlatBufferBuilder, matchType: Byte) = builder.addByte(1, matchType, 0)
        fun addAnnotaionCount(builder: FlatBufferBuilder, annotaionCount: Int) = builder.addOffset(2, annotaionCount, 0)
        fun endAnnotationsMatcher(builder: FlatBufferBuilder) : Int {
            val o = builder.endTable()
            return o
        }
    }
}

@file:Suppress("MemberVisibilityCanBePrivate", "unused")

package org.luckypray.dexkit

import com.google.flatbuffers.FlatBufferBuilder
import io.luckypray.dexkit.annotations.DexKitExperimentalApi
import io.luckypray.dexkit.builder.BatchFindArgs
import io.luckypray.dexkit.builder.ClassUsingAnnotationArgs
import io.luckypray.dexkit.builder.FieldUsingAnnotationArgs
import io.luckypray.dexkit.builder.FindClassArgs
import io.luckypray.dexkit.builder.FindMethodArgs
import io.luckypray.dexkit.builder.MethodCallerArgs
import io.luckypray.dexkit.builder.MethodInvokingArgs
import io.luckypray.dexkit.builder.MethodOpcodeArgs
import io.luckypray.dexkit.builder.MethodUsingAnnotationArgs
import io.luckypray.dexkit.builder.MethodUsingFieldArgs
import io.luckypray.dexkit.builder.MethodUsingNumberArgs
import io.luckypray.dexkit.builder.MethodUsingStringArgs
import io.luckypray.dexkit.descriptor.member.DexClassDescriptor
import io.luckypray.dexkit.descriptor.member.DexFieldDescriptor
import io.luckypray.dexkit.descriptor.member.DexMethodDescriptor
import io.luckypray.dexkit.enums.FieldUsingType
import io.luckypray.dexkit.enums.MatchType
import io.luckypray.dexkit.util.OpCodeUtil.getOpFormat
import org.luckypray.dexkit.schema.BatchFindClassUsingStrings
import org.luckypray.dexkit.schema.BatchFindMethodUsingStrings
import org.luckypray.dexkit.schema.FindClass
import org.luckypray.dexkit.schema.FindMethod
import java.io.Closeable

class DexKitBridge : Closeable {

    private var token: Long = 0L

    private constructor(apkPath: String) {
        token = nativeInitDexKit(apkPath)
    }

    private constructor(dexBytesArray: Array<ByteArray>) {
        token = nativeInitDexKitByBytesArray(dexBytesArray)
    }

    private constructor(classLoader: ClassLoader, useMemoryDexFile: Boolean) {
        token = nativeInitDexKitByClassLoader(classLoader, useMemoryDexFile)
    }

    /**
     * DexKit is valid only when token is not 0
     */
    val isValid
        get() = token != 0L

    /**
     * release native resource
     */
    @Synchronized
    override fun close() {
        if (isValid) {
            nativeRelease(token)
            token = 0L
        }
    }

    /**
     * set DexKit work thread number
     *
     * @param [num] work thread number
     */
    fun setThreadNum(num: Int) {
        nativeSetThreadNum(token, num)
    }

    /**
     * Get all parsed dex counts.
     *
     * @return The number of dex parsed by DexKit
     */
    fun getDexNum(): Int {
        return nativeGetDexNum(token)
    }

    /**
     * write all dex file to [outPath]
     *
     * @param [outPath] dex file output path
     *
     * @since 1.1.0
     */
    fun exportDexFile(outPath: String) {
        nativeExportDexFile(token, outPath)
    }

    /**
     * find class by [BatchFindMethodUsingStrings]'s [FlatBufferBuilder]
     */
    internal fun batchFindClassUsingStrings(fbb: FlatBufferBuilder) {
        nativeBatchFindClassUsingStrings(token, fbb.sizedByteArray())
    }

    /**
     * find class by [BatchFindClassUsingStrings]'s [FlatBufferBuilder]
     */
    internal fun batchFindMethodUsingStrings(fbb: FlatBufferBuilder) {
        nativeBatchFindMethodUsingStrings(token, fbb.sizedByteArray())
    }

    /**
     * find class by [FindClass]'s [FlatBufferBuilder]
     */
    internal fun findClass(fbb: FlatBufferBuilder) {
        nativeFindClass(token, fbb.sizedByteArray())
    }

    /**
     * find method by [FindMethod]'s [FlatBufferBuilder]
     */
    internal fun findMethod(fbb: FlatBufferBuilder) {
        nativeFindMethod(token, fbb.sizedByteArray())
    }

    /**
     * find method by [FindMethod]'s [FlatBufferBuilder]
     */
    internal fun findField(fbb: FlatBufferBuilder) {
        nativeFindField(token, fbb.sizedByteArray())
    }

    companion object {

        @JvmStatic
        fun create(apkPath: String): DexKitBridge? {
            val helper = DexKitBridge(apkPath)
            return if (helper.isValid) helper else null
        }

        @JvmStatic
        fun create(dexBytesArray: Array<ByteArray>): DexKitBridge? {
            val helper = DexKitBridge(dexBytesArray)
            return if (helper.isValid) helper else null
        }

        /**
         *
         * @param loader class loader
         * @param useMemoryDexFile
         * if true, will try to use cookie to load memory dex file. else will use dex file path.
         * if cookies file contains CompactDex, will use apkPath to load dex.
         * if contains OatDex, Some functions may not work properly.
         */
        @JvmStatic
        fun create(loader: ClassLoader, useMemoryDexFile: Boolean): DexKitBridge? {
            val helper = DexKitBridge(loader, useMemoryDexFile)
            return if (helper.isValid) helper else null
        }

        @JvmStatic
        private external fun nativeInitDexKit(apkPath: String): Long

        @JvmStatic
        private external fun nativeInitDexKitByBytesArray(dexBytesArray: Array<ByteArray>): Long

        @JvmStatic
        private external fun nativeInitDexKitByClassLoader(
            loader: ClassLoader,
            useMemoryDexFile: Boolean
        ): Long

        @JvmStatic
        private external fun nativeSetThreadNum(nativePtr: Long, threadNum: Int)

        @JvmStatic
        private external fun nativeGetDexNum(nativePtr: Long): Int

        @JvmStatic
        private external fun nativeRelease(nativePtr: Long)

        @JvmStatic
        private external fun nativeExportDexFile(nativePtr: Long, outDir: String)

        @JvmStatic
        private external fun nativeBatchFindClassUsingStrings(nativePtr: Long, bytes: ByteArray)

        @JvmStatic
        private external fun nativeBatchFindMethodUsingStrings(nativePtr: Long, bytes: ByteArray)

        @JvmStatic
        private external fun nativeFindClass(nativePtr: Long, bytes: ByteArray)

        @JvmStatic
        private external fun nativeFindMethod(nativePtr: Long, bytes: ByteArray)

        @JvmStatic
        private external fun nativeFindField(nativePtr: Long, bytes: ByteArray)

    }

    protected fun finalize() {
        close()
    }
}

#include "Bitmap.h"

Bitmap::Bitmap(int w, int h, int comp, eBitmapFormat fmt)
	: mWidth(w), mHeight(h), mComp(comp), mFmt(fmt), mData(w * h * comp * getBytesPerComponent(fmt))
{
	initGetSetFuncs();
}

Bitmap::Bitmap(int w, int h, int comp, eBitmapFormat fmt, const void* ptr)
	: Bitmap(w, h, comp, fmt)
{
	memcpy(mData.data(), ptr, mData.size());
}

Bitmap::Bitmap(int w, int h, int d, int comp, eBitmapFormat fmt)
	: mWidth(w), mHeight(h), mDepth(d), mComp(comp), mFmt(fmt), mData(w * h * d * comp * getBytesPerComponent(fmt))
{
	initGetSetFuncs();
}

void Bitmap::setPixel(int x, int y, const glm::vec4& c)
{
	(*this.*mSetPixelFunc)(x, y, c);
}

glm::vec4 Bitmap::getPixel(int x, int y) const
{
	return (*this.*mGetPixelFunc)(x, y);
}

void Bitmap::initGetSetFuncs()
{
	// choose the proper getter and setter based on the format of the bitmap 
	switch (mFmt)
	{
		case eBitmapFormat::BitMapUnsignedByte:
			mSetPixelFunc = &Bitmap::setPixelUnsignedByte;
			mGetPixelFunc = &Bitmap::getPixelUnsignedByte;
			break;
		case eBitmapFormat::BitMapFloat:
			mSetPixelFunc = &Bitmap::setPixelFloat;
			mGetPixelFunc = &Bitmap::getPixelFloat;
			break;
	}
}

void Bitmap::setPixelUnsignedByte(int x, int y, const glm::vec4& c)
{
	const int offsets = mComp * (y * mWidth + x);
	if (mComp > 0) mData[offsets + 0] = uint8_t(c.x * 255.0f);
	if (mComp > 1) mData[offsets + 1] = uint8_t(c.y * 255.0f);
	if (mComp > 2) mData[offsets + 2] = uint8_t(c.z * 255.0f);
	if (mComp > 3) mData[offsets + 3] = uint8_t(c.w * 255.0f);
}

glm::vec4 Bitmap::getPixelUnsignedByte(int x, int y) const
{
	const int offsets = mComp * (y * mWidth + x);
	return {
		mComp > 0 ? float(mData[offsets + 0]) / 255.0f : 0.0f,
		mComp > 1 ? float(mData[offsets + 1]) / 255.0f : 0.0f,
		mComp > 2 ? float(mData[offsets + 2]) / 255.0f : 0.0f,
		mComp > 3 ? float(mData[offsets + 3]) / 255.0f : 0.0f
	};
}

void Bitmap::setPixelFloat(int x, int y, const glm::vec4& c)
{
	const int offsets = mComp * (y * mWidth + x);
	float*    data    = reinterpret_cast<float*>(mData.data());
	if (mComp > 0) data[offsets + 0] = c.x;
	if (mComp > 1) data[offsets + 1] = c.y;
	if (mComp > 2) data[offsets + 2] = c.z;
	if (mComp > 3) data[offsets + 3] = c.w;
}

glm::vec4 Bitmap::getPixelFloat(int x, int y) const
{
	const int    offsets = mComp * (y * mWidth + x);
	const float* data    = reinterpret_cast<const float*>(mData.data());
	return {
		mComp > 0 ? data[offsets + 0] : 0.0f,
		mComp > 1 ? data[offsets + 1] : 0.0f,
		mComp > 2 ? data[offsets + 2] : 0.0f,
		mComp > 3 ? data[offsets + 3] : 0.0f
	};
}

int Bitmap::getBytesPerComponent(eBitmapFormat fmt)
{
	if (fmt == eBitmapFormat::BitMapUnsignedByte) return 1;
	if (fmt == eBitmapFormat::BitMapFloat) return 4;
	return 0;
}

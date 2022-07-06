#pragma once
#include <vector>
#include <glm/glm.hpp>

enum class eBitmapFormat
{
	BitMapUnsignedByte,
	BitMapFloat,
};

enum class eBitmapType
{
	BitMap2D,
	BitMapCube
};

class Bitmap
{
public:
	Bitmap() = default;
	Bitmap(int w, int h, int comp, eBitmapFormat fmt);
	Bitmap(int w, int h, int comp, eBitmapFormat fmt, const void* ptr);
	Bitmap(int w, int h, int d, int comp, eBitmapFormat fmt);

	void       setPixel(int x, int y, const glm::vec4& c);
	glm::vec4  getPixel(int x, int y) const;
	static int getBytesPerComponent(eBitmapFormat fmt);

	int                  mWidth  = 0;
	int                  mHeight = 0;
	int                  mDepth  = 1;
	int                  mComp   = 3;
	eBitmapFormat        mFmt    = eBitmapFormat::BitMapUnsignedByte;
	eBitmapType          mType   = eBitmapType::BitMap2D;
	std::vector<uint8_t> mData;
private:
	// initialize setter and getter functions based on its format
	void initGetSetFuncs();

	// list of supported setters and getters:
	void      setPixelUnsignedByte(int x, int y, const glm::vec4& c);
	glm::vec4 getPixelUnsignedByte(int x, int y) const;

	void      setPixelFloat(int x, int y, const glm::vec4& c);
	glm::vec4 getPixelFloat(int x, int y) const;

	// define aliases for setter and getter's signature
	using setPixel_t = void(Bitmap::*)(int, int, const glm::vec4&);
	using getPixel_t = glm::vec4(Bitmap::*)(int, int) const;

	// each bitmap has a set of setter and getter dynamically chosen based on its format
	// by default, we use unsigned byte bit map
	setPixel_t mSetPixelFunc = &Bitmap::setPixelUnsignedByte;
	getPixel_t mGetPixelFunc = &Bitmap::getPixelUnsignedByte;
};

#ifndef __GLYPH_H__
#define __GLYPH_H__
#include <stdint.h>
#include <memory>
#include <vector>
#include <cassert>
#include <PhBase/PhBase.h>
#include <graphics/TexOGL.h>
#include <algorithm>
#include <xhash>
#include <unordered_map>
#include <ft2build.h>
#include <freetype/freetype.h>
//#include "flat_hash_map.hpp"

#define INVALID_CANVAS_UNIT 0xffff

namespace ph
{
	// GlyphRenderDesc �����ü����� Widget���������������
	// ���ɶ������ݵ�ʱ�򣬾͵�����widget���پ͵ݼ�
	// widget����Ҫ�洢 GlyphRenderDesc�����ã�ֱ�Ӵ�unicode + �ֺŴ�С������GlyphDesc��ӳ�䵽GlyphRenderDesc���󣬷������ü���
	struct GlyphRenderDesc
	{
		// λͼ������߼���Ⱦλ��ƫ��
		PhI16        penx; 
		PhI16        peny;
		// ��Ⱦ����������ʵ��ռ�õĴ�С
		PhU16        width;
		PhU16        height;
		// ��Ⱦ�ã�ָ������
		PhU8         canvasId;
		// ������
		PhU8		 subcanvasId;
		PhU8		 unitId;
		// ��������
		float		tcxMin;
		float		tcxMax;
		float		tcyMin;
		float		tcyMax;
		// λͼ��ʵ�ʴ�С��ʵ���ò�����
		PhU16		 bmpWidth;
		PhU16		 bmpHeight;
		// λͼ�������ڵ�λ�ã�ʵ��Ҳ�ò���
		PhU16        canvasX;
		PhU16        canvasY;
	
	};

	typedef  std::shared_ptr<GlyphRenderDesc> GRDRef;

	struct GlyphDesc
	{
		uint16_t        unicode;    // unicode ����
		uint16_t        bound;      // �ֺ�
		/*GlyphDesc& operator = (const GlyphDesc & _desc )
		{
			this->unicode = _desc.unicode;
			this->bound = _desc.bound;
			return *this;
		}*/
	};
	typedef GlyphDesc GD;


	struct GlyphDescHash
	{
		size_t operator()(const GlyphDesc &_gd) const
		{
			size_t hash = 5381;
			for (size_t i = 0; i < sizeof(_gd); i++)
			{
				hash = ((hash << 5) + hash) + *(((char*)&_gd) + i);
			}
			return hash;
		}
	};

	class GlyphDescEqual
	{
	public:
		bool operator()(const GlyphDesc& _dsc1, const GlyphDesc& _dsc2) const
		{
			return memcmp(&_dsc1, &_dsc2, sizeof(GlyphDesc)) == 0;
		}
	};
}

namespace ph
{
	struct SubCanvas
	{
	private:
		PhU16       width;
		PhU16       height;

		PhU16       fitMax;      // unit size
		PhU16		fitMin;

		PhU16       logicX;
		PhU16       logicY;

		PhU16		baseX;
		PhU16		baseY;

		std::vector< PhU16 > vecFree;
	public:
		size<PhU16>	GetSize()
		{
			return size<PhU16>(width, height);
		}

		PhU16 GetUnitSize()
		{
			return  fitMax;
		}

		bool CanAlloc()
		{
			if (!vecFree.empty())
				return true;
			if (logicX + fitMax <= width)
				return true;
			if (logicY < height - fitMax)
				return true;
			return false;
		}

		SubCanvas(PhU16  _width, PhU16 _height, PhU16 _unitSize, PhU16 _baseX, PhU16 _baseY)
		{
			assert(_unitSize == 8 || _unitSize == 16 || _unitSize == 32 || _unitSize == 64 || _unitSize == 128 || _unitSize == 256);
			width = _width;
			height = _height;
			fitMax = _unitSize;
			if (_unitSize == 8)
				fitMin = 1;
			else
				fitMin = fitMax >> 1;
			logicX = logicY = 0;
			baseX = _baseX;
			baseY = _baseY;
		}

		bool IsFit(PhU16 _size)
		{
			if (_size > this->fitMax) return false;
			if (_size > 4 && _size * 2 <= this->fitMax) return false;
			return true;
		}

		PhU16 Alloc(PhU16& _globalX, PhU16& _globalY)
		{
			if (!vecFree.empty())
			{
				PhU16 id = vecFree.back();
				vecFree.pop_back();
				_globalX = baseX + id % (width / fitMax) * fitMax;
				_globalY = baseY + id / (width / fitMax) * fitMax;
				printf("alloc from vecFree : id = %d\n unitsize : %d", id, this->fitMax);
				return id;
			}
			if (logicX+fitMax > width)
			{
				logicX = 0; logicY += fitMax;
				if (logicY >= height) return INVALID_CANVAS_UNIT;
			}
			//_x = logicX;
			//_y = logicY;
			PhU16 id = (logicX / fitMax) + (logicY / fitMax) * (width / fitMax);
			_globalX = logicX + baseX;
			_globalY = logicY + baseY;
			printf("alloc from unused content : id = %d unitsize = %d x = %d y = %d localX = %d localY = %d \n", id, this->fitMax, _globalX, _globalY, logicX, logicY);
			logicX += fitMax;
			return  id;
		}

		bool Free(PhU16 _id)
		{
			for (const auto theId : vecFree)
			{
				if (theId == _id)
					return false;
			}
			vecFree.push_back(_id);
			return true;
		}
	};

	inline PhU16 LeftSizeFitUnit(PhU16 _left)
	{
		if (_left == 256) return 64;
		if (_left < 256 && _left >= 128) return 32;
		if (_left < 128 && _left >= 64) return 16;
		if (_left < 64 && _left >0) return 8;
		return 0;
	}


	inline PhU8 FitSize2Unitsize(PhU16 _left)
	{
		PhU32 i = 1;
		while (i < _left)
		{
			i = i << 1;
		}
		return i;
	}

	inline PhU16 GetDefSubCanvasWidth(PhU16 _unitSize)
	{
		switch (_unitSize)
		{
		case 256:return 1024;
		case 128:return 512;
		case 64: return 256;
		case 32: return 128;
		case 16: return 64;
		case 8: return 32;
		default: return 1024;
		}
	}

	inline SubCanvas CreateSubCanvas(PhU16 _width, PhU16 _unitSize, PhU16 _baseX, PhU16 _baseY)
	{
		SubCanvas canvas(_width, 256, _unitSize, _baseX, _baseY);
		return std::move(canvas);
	}

	struct Canvas
	{
		PhU16       width;
		PhU16       height;

		PhU16       logicX;
		PhU16       logicY;

		TexOGLRef	texture;

		std::vector<SubCanvas> vecSubCanvas;

		Canvas(PhU16 _width, PhU16 _height)
		{
			texture = std::shared_ptr<TexOGL>(TexOGL::CreateTexEmpty( PIXEL_FORMAT_A8 , _width, _height), [](TexOGL* _ptr) {
			_ptr->Release();
			});			
			/*SamplerState ss;
			ss.MagFilter = TEX_FILTER_POINT;
			ss.MinFilter = TEX_FILTER_POINT;
			texture->sampler.SetDesc(ss);*/
			logicX = logicY = 0;
			width = _width;
			height = _height;
		}

		bool Alloc(PhU16 _width, PhU16 _height, PhU16& _logicX, PhU16& _logicY, PhU8& _subId, PhU8& _unitId)
		{
			PhU16 fitSize = _width > _height ? _width : _height;
			// ������û�п��е�
			for (size_t i = 0; i < vecSubCanvas.size(); ++i)
			{
				auto& child = vecSubCanvas[i];
				if (child.IsFit(fitSize))
				{
					if (child.CanAlloc())
					{
						PhU16 allocX, allocY;
						_unitId = (PhU8)child.Alloc(allocX, allocY);
						if (_unitId != INVALID_CANVAS_UNIT)
						{
							_subId = (PhU8)i;
							_logicX = allocX;
							_logicY = allocY;
							return true;
						}
					}
				}
			}
			// û�п��е�
			// ���ŷ���, �߶�ͳһΪ256,����ֻ�жϿ���ǲ��ǹ�������
			//PhU16 baseX, baseY;
			PhU16 widthLeft = width - logicX;
			PhU16 heightLeft = height - logicY - 256;


			PhU8 allocUnitSize = FitSize2Unitsize(fitSize);

			// ��һ�в���һ��ָ���ַ��Ŀ��ˣ�����Ҫ��ʣ������õ����ܰ����и����䣬���û����һ���ˣ��Ͳ�Ҫǿ��������
			if (widthLeft < fitSize)
			{
				if (heightLeft == 0)
				{
					return false;
				}
				assert(heightLeft >= 256);
				// �߶�ʣ����ȫ���Է����κδ�С������
				// �����ұߵ�ʣ��ռ���Ȼ��������
				PhU16 reuseUnitSize = LeftSizeFitUnit(widthLeft);
				if( reuseUnitSize != 0)
				{
					SubCanvas canvas(widthLeft, 256, reuseUnitSize, logicX, logicY);
					vecSubCanvas.push_back(std::move(canvas));
				}

				// ��������
				logicX = 0;
				logicY += 256;


				PhU16 defCanvasWidth = GetDefSubCanvasWidth(allocUnitSize);
				assert(defCanvasWidth < this->width);
				SubCanvas& canvas = CreateSubCanvas(defCanvasWidth, allocUnitSize, logicX, logicY);
				vecSubCanvas.push_back(std::move(canvas));
				//assert(&canvas == &(*vecSubCanvas.end()));
				_subId = (PhU8)vecSubCanvas.size() - 1;
				_unitId = (PhU8)vecSubCanvas[vecSubCanvas.size() - 1].Alloc(_logicX, _logicY);
				logicX += defCanvasWidth;
				return true;
			}
			else // ��ǰ��ʣ������ȫ���ã�����һ���ַ��Ŀ���ǹ��õģ�
			{
				PhU16 defCanvasWidth = GetDefSubCanvasWidth(allocUnitSize);
				PhU16 subCanvasWidth = defCanvasWidth > widthLeft ? widthLeft : defCanvasWidth;
				SubCanvas& canvas = CreateSubCanvas(subCanvasWidth, allocUnitSize, logicX, logicY);
				vecSubCanvas.push_back(std::move(canvas));
				//canvas = vecSubCanvas[vecSubCanvas.size()-1];
				//assert(&canvas == &canvasRef);
				_subId = (PhU8)vecSubCanvas.size() - 1;
				_unitId = (PhU8)vecSubCanvas[vecSubCanvas.size() - 1].Alloc(_logicX, _logicY);
				logicX += subCanvasWidth;
				return true;
			}
		}

		void WritePixel(PhU16 _x, PhU16 _y, PhU16 _w, PhU16 _h, PhU8 * _data)
		{
			glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
			this->texture->BufferSubData(_x, _y, _w, _h, _data);
		}
	};

    class Glypher
    {
    private:
		std::unordered_map< const GlyphDesc, std::shared_ptr<GlyphRenderDesc>, GlyphDescHash, GlyphDescEqual >	glyphMap;
		std::vector<PhU8>																						heightTable;
        //ska::flat_hash_map< const GlyphDesc&, std::shared_ptr<GlyphRenderDesc> >	glyphMap;
		std::vector<Canvas*>																					vecCanvas;
		IBlob *																									fontBlob;
		FT_Library 																								library;
		FT_Face 																								face;
    public:
        Glypher();
        ~Glypher();
		bool Init( const char * _fontFile );
		GRDRef Get(const GD& _desc);
		PhU16 FaceHeight( PhU16 _bound );
		TexOGLRef GetCanvasTexRef(PhSizeT _index);
    private:
       // void kickLazyGlyph();
    };

}

#endif // __GLYPH_H__

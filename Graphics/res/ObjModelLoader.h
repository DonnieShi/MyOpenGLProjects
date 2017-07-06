#pragma once

#include <PhBase/Archive.h>
#include <Graphics/BufferOGL.h>
#include <Graphics/TexOGL.h>
#include <vector>
#include <res/TexPool.h>


namespace ph
{
	struct ObjVertData
	{
		vec3	pos;
		vec3	norm;
		vec2	tex;
		ObjVertData()
		{
			pos.x = pos.y = pos.z = 0.0f;
			norm.x = norm.y = norm.z = 0.0f;
			tex.x = tex.y = 0.0f;
		}
	};

	struct Material
	{
		char			name[32];
		vec3			ambient; // ������
		vec3			diffuse; // ɢ���
		vec3			specular; // ���淴����ɫ
		float			shiness; // ���淴��ָ��
		TexOGLRef		texAmbient;
		TexOGLRef		texDiffuse;
		TexOGLRef		texHighlight;
	};

	struct ClientSideMesh
	{
		std::vector< ObjVertData >	vertices;
		std::vector< unsigned int>  indices;
		TexOGLRef					tex;
		std::string					mtl;
	};

	struct ServerSideMesh
	{
		IBindable*		vbo;
		IBindable*		ibo;
		VertexArray*	vao;
		size_t			nElement;
		size_t			material;
	};

	typedef std::vector< Material >			MaterialVector;
	typedef ClientSideMesh					ClientMesh;
	typedef ServerSideMesh					Mesh;
	typedef std::vector< ClientSideMesh >	ClientMeshVector;
	typedef std::vector< ServerSideMesh >	MeshVector;

	struct Model3D
	{
		MeshVector			vecMesh;
		MaterialVector		vecMaterial;
	};

	inline ServerSideMesh CreateServerSideMesh(ClientSideMesh& _cm, MaterialVector& _materialVec )
	{
		ServerSideMesh mesh;
		StaticVB * vbo = StaticVB::New(_cm.vertices.data(), _cm.vertices.size() * sizeof(ObjVertData));
		StaticIB * ibo = StaticIB::New(_cm.indices.data(), _cm.indices.size() * sizeof(unsigned int));
		static VertexArray::Layout layout[] = 
		{
			{0, 3, GL_FLOAT, sizeof(ObjVertData), 0 },
			{1, 3, GL_FLOAT, sizeof(ObjVertData), (GLvoid*) (sizeof(float) * 3) },
			{2, 2, GL_FLOAT, sizeof(ObjVertData), (GLvoid*) (sizeof(float) * 6) },
			{0}
		};
		VertexArray * vao = VertexArray::New(vbo, ibo, &layout[0]);
		mesh.ibo = ibo;
		mesh.vbo = vbo;
		mesh.vao = vao;
		mesh.material = 0;
		mesh.nElement = _cm.indices.size();
		for (size_t i = 0; i < _materialVec.size(); ++i)
		{
			if (_cm.mtl == _materialVec[i].name)
			{
				mesh.material = i;
				break;
			}
		}
		return mesh;
	}

	inline Model3D CreateModel3D(ClientMeshVector& _cmv, MaterialVector& _mv)
	{
		Model3D m;
		for (auto& _cm : _cmv)
		{
			m.vecMesh.push_back(CreateServerSideMesh(_cm, _mv));
		}
		m.vecMaterial = _mv;
		return m;
	}

	inline bool LoadObjModel(const char * _model, ClientMeshVector& _data, MaterialVector& _vecMaterial);

	inline Model3D CreateModel3D(const char * _objFile)
	{
		ClientMeshVector cmv;
		MaterialVector mv;
		bool ret = LoadObjModel(_objFile, cmv, mv);
		if (ret)
		{
			return CreateModel3D(cmv, mv);
		}
		return Model3D();
	}

	inline bool LoadObjMtl(const char * _mtl, std::vector<Material>& _material )
	{
		Archive * arch = GetDefArchive();
		IBlob * mtlBlob = arch->Open(_mtl);
		if (!mtlBlob)
		{
			return false;
		}
		const char * ptr = mtlBlob->GetBuffer();
		char ReadSlots[8][64] = { 0 };

		std::string dirPath = mtlBlob->Filepath();
		dirPath = Archive::FormatFilePath(dirPath);
		size_t pos = dirPath.rfind('/');
		if (pos != std::string::npos)
		{
			dirPath = std::string(&dirPath[0], &dirPath[pos]);
			dirPath.push_back('/');
		}
		else
		{
			dirPath = "";
		}

		Material * current = nullptr;

		std::string line;

		while (*ptr)
		{
			const char * pos = strchr(ptr, '\n');
			if (pos)
				line.assign(ptr, pos);
			else
				line = ptr;
			int ret = sscanf(line.c_str(), "%s %s %s %s %s %s %s %s",ReadSlots[0],ReadSlots[1],ReadSlots[2],ReadSlots[3], ReadSlots[4],ReadSlots[5],ReadSlots[6],ReadSlots[7]);
			if (ret > 1)
			{
				if (strcmp(ReadSlots[0], "#") == 0) // ע����������ȡ��һ��
				{
				}
				else if (strcmp(ReadSlots[0], "newmtl") == 0) // �²�������
				{
					Material mater;
					mater.texHighlight = mater.texDiffuse = mater.texAmbient = TexPool::GetWhite();
					_material.push_back(mater);
					current = &_material.back();
					strncpy(current->name, ReadSlots[1], 32);
				}
				else if (strcmp(ReadSlots[0], "Ka") == 0) // ������
				{
					sscanf(ReadSlots[1], "%f", &current->ambient.x);
					sscanf(ReadSlots[2], "%f", &current->ambient.y);
					sscanf(ReadSlots[3], "%f", &current->ambient.z);
				}
				else if (strcmp(ReadSlots[0], "Kd") == 0) // ɢ���
				{
					sscanf(ReadSlots[1], "%f", &current->diffuse.x);
					sscanf(ReadSlots[2], "%f", &current->diffuse.y);
					sscanf(ReadSlots[3], "%f", &current->diffuse.z);
				}
				else if (strcmp(ReadSlots[0], "Ks") == 0) // �������ɫ
				{
					sscanf(ReadSlots[1], "%f", &current->specular.x);
					sscanf(ReadSlots[2], "%f", &current->specular.y);
					sscanf(ReadSlots[3], "%f", &current->specular.z);
				}
				else if (strcmp(ReadSlots[0], "Ns") == 0) // �������
				{
					sscanf(ReadSlots[1], "%f", &current->shiness);
				}
				else if (strcmp(ReadSlots[0], "map_Ka") == 0) // ��������ͼ
				{
					std::string texFile = dirPath + ReadSlots[1];
					texFile = Archive::FormatFilePath( texFile );
					current->texAmbient = TexPool::Get(texFile.c_str());
				}
				else if (strcmp(ReadSlots[0], "map_Kd") == 0) // ɢ�����ͼ
				{
					std::string texFile = dirPath + ReadSlots[1];
					texFile = Archive::FormatFilePath(texFile);
					current->texDiffuse = TexPool::Get(texFile.c_str());
				}
				else if (strcmp(ReadSlots[0], "map_Ks") == 0) // �������ͼ
				{
					std::string texFile = dirPath + ReadSlots[1];
					texFile = Archive::FormatFilePath(texFile);
					current->texHighlight = TexPool::Get(texFile.c_str());
				}
			}
			// ��һ��
			while (*ptr != '\n') { 
				if (!*ptr) 
					return true;	
				++ptr;
			}
			++ptr;
		}
		return true;
	}

	bool LoadObjModel(const char * _model, ClientMeshVector& _data, MaterialVector& _vecMaterial)
	{
		Archive * arch = GetDefArchive();
		IBlob * objBlob = arch->Open(_model);

		std::string fullpath = arch->GetRoot();
		fullpath.append(_model);
		fullpath = Archive::FormatFilePath(fullpath);

		char ReadSlots[8][64] = {0};

		bool mtlRet = false;
		std::vector< Material >&	vecMaterial = _vecMaterial;
		if (!objBlob)
		{
			return false;
		}
		// ���㼯
		std::vector<vec3> vecVert;
		std::vector<vec2> vecCoord;
		std::vector<vec3> vecNorm;

		// ģ�����Ǽ�
		std::vector<ObjVertData> shape;

		const char * ptr = objBlob->GetBuffer();

		ClientSideMesh * curModel = nullptr;

		std::string line;

		while (*ptr)
		{
			const char * pos = strchr(ptr, '\n');
			if (pos)
				line.assign(ptr, pos);
			else
				line = ptr;

			int ret = sscanf(line.c_str(), "%s %s %s %s %s %s %s %s", ReadSlots[0], ReadSlots[1], ReadSlots[2], ReadSlots[3], ReadSlots[4], ReadSlots[5], ReadSlots[6], ReadSlots[7]);
			if (ret > 1 )
			{
				if (strcmp(ReadSlots[0], "#") == 0) // ע����������ȡ��һ��
				{
				}
				else if (strcmp(ReadSlots[0], "mtllib") == 0) // �����ļ�
				{
					size_t pos = fullpath.rfind('/');
					if (pos == std::string::npos)
					{
						mtlRet = LoadObjMtl(ReadSlots[1], vecMaterial);
					}
					else
					{
						std::string objDir = std::string(&fullpath[0], &fullpath[fullpath.rfind('/')]);
						std::string mtlFilepath = objDir;
						mtlFilepath.push_back('/');
						mtlFilepath.append(ReadSlots[1]);
						mtlRet = LoadObjMtl(mtlFilepath.c_str(), vecMaterial);
					}					
				}
				else if (strcmp(ReadSlots[0], "v") == 0) // ����xyz
				{
					vec3 vert;
					sscanf(ReadSlots[1], "%f", &vert.x);
					sscanf(ReadSlots[2], "%f", &vert.y);
					sscanf(ReadSlots[3], "%f", &vert.z);
					vecVert.push_back(vert);
				}
				else if (strcmp(ReadSlots[0], "vn") == 0) // ���㷨��
				{
					vec3 norm;
					sscanf(ReadSlots[1], "%f", &norm.x);
					sscanf(ReadSlots[2], "%f", &norm.y);
					sscanf(ReadSlots[3], "%f", &norm.z);
					vecNorm.push_back(norm);
				}
				else if (strcmp(ReadSlots[0], "vt") == 0) // ������������
				{
					vec2 tc;
					sscanf(ReadSlots[1], "%f", &tc.x);
					sscanf(ReadSlots[2], "%f", &tc.y);
					vecCoord.push_back(tc);
				}
				else if (strcmp(ReadSlots[0], "usemtl") == 0) // ����ָ��
				{
					// �����µ�
					ClientSideMesh ms;
					_data.push_back(ms);
					curModel = &_data.back();
					curModel->mtl = ReadSlots[1];
				}
				else if (strcmp(ReadSlots[0], "f") == 0) // ������
				{
					ObjVertData ovd[4];

					size_t faceParamN = ret - 1;

					int vd, td, nd;
					for (size_t i = 0; i < faceParamN; ++i)
					{
						int r = sscanf(ReadSlots[i+1], "%d/%d/%d", &vd, &td, &nd);
						switch (r)
						{
						case 3:
							ovd[i].norm = vecNorm[nd-1];
						case 2:
							ovd[i].tex = vecCoord[td-1];
						case 1:
							ovd[i].pos = vecVert[vd-1];
							break;
						default:
							break;
						}
						curModel->vertices.push_back(ovd[i]);
					}
					unsigned int indicesN = curModel->vertices.size();
					indicesN -= faceParamN;
					if (faceParamN == 3) // һ��������
					{
						curModel->indices.push_back(indicesN);
						curModel->indices.push_back(indicesN + 1);
						curModel->indices.push_back(indicesN + 2);
					}
					else if (faceParamN == 4) // ����������
					{
						curModel->indices.push_back(indicesN);
						curModel->indices.push_back(indicesN + 1);
						curModel->indices.push_back(indicesN + 2);
						curModel->indices.push_back(indicesN);
						curModel->indices.push_back(indicesN + 2);
						curModel->indices.push_back(indicesN + 3);
					}
				}
			}
			// ��һ��
			while (*ptr != '\n') {
				if (!*ptr)
					return true;
				++ptr;
			}
			++ptr;
		}
		return true;
	}

}


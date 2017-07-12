#pragma once

#include <PhBase/PhBase.h>
#include <render/Glypher.h>
#include <graphics/RenderStateGL.h>
#include <BufferOGL.h>
#include <TexOGL.h>
#include <res/ObjModelLoader.h>
#include <glm/glm.hpp>

namespace ph
{

	class Basic3DRender
	{
	private:
		ShaderOGL *				shader;				// ��ɫ��
		UniformBufferObjectRef	transUBO;			// ��ɫ����UBO
		//
		SamplerSlotRef			diffuseTexSlot;		// ɢ���������
		SamplerSlotRef			ambientTexSlot;		// �������������
		SamplerSlotRef			specTexSlot;		// �߹��������

		RenderStateGL			renderState;		// ��Ⱦ״̬
		
		glm::mat4x4				matProj;			// ͶӰ����
		glm::mat4x4				matView;			// ��ͼ����(�����λ���Լ�����)

		glm::vec4				lightPos;			// ����Դλ��
		glm::vec4				lightColor;			// ����Դ��ɫ

	public:

		bool Init(Archive * _arch)
		{
			// ��ʼ��shader
			IBlob * vert = _arch->Open("Basic3D.vert");
			IBlob * frag = _arch->Open("Basic3D.frag");
			shader = ShaderOGL::CreateShader(vert->GetBuffer(), frag->GetBuffer());
			if (!shader)
			{
				return false;
			}
			vert->Release();
			frag->Release();
			// ��ȡ����󶨲�
			diffuseTexSlot = shader->GetSamplerSlot("diffuse_texture");
			ambientTexSlot = shader->GetSamplerSlot("ambient_texture");
			specTexSlot = shader->GetSamplerSlot("specular_texture");
			ph::SamplerState ss;
			ss.MagFilter = ph::TEX_FILTER_POINT;
			ss.MinFilter = ph::TEX_FILTER_POINT;
			this->diffuseTexSlot->SetSamplerState(ss);
			// ��ȡUBO
			transUBO = shader->AllocUniformBufferObject("RenderParam", 0);
			// ������Ⱦ������Ⱦ״̬
			RenderState rs;
			rs.cullMode = CULL_MODE_NONE;
			rs.depthTestable = true;
			renderState = RenderStateGL(rs);
			// ����Ĭ�ϵĵƹ�
			lightPos = glm::vec4(0, 0, 0, 1);
			lightColor = glm::vec4(1.0, 1.0, 1.0, 1);

			return true;
		}


		void SetLight(const glm::vec4& _pos, const glm::vec4& _color)
		{
			lightPos = _pos;
			lightColor = _color;
		}

		bool Begin(const glm::mat4x4& _matView, const glm::mat4x4& _matProj, const glm::vec4& _eye )
		{
			// Ӧ���������
			matProj = _matProj;
			matView = _matView;
			// Ӧ����ɫ�����󶨣�
			shader->Bind();
			// �󶨶�Ӧ��UBO
			transUBO->Bind();
			// ������Ⱦ״̬
			renderState.Apply();
			// ����UBO��Ӧ�ľ�ͷ����
			transUBO->WriteData(&matProj, 0, sizeof(glm::mat4x4));
			transUBO->WriteData(&matView, sizeof(glm::mat4x4), sizeof(glm::mat4x4));

			transUBO->WriteData(&lightPos, sizeof(glm::mat4x4) * 3, sizeof(glm::vec4));
			transUBO->WriteData(&lightColor, sizeof(glm::mat4x4) * 3 + sizeof(glm::vec4), sizeof(glm::vec4));
			transUBO->WriteData(&_eye, sizeof(glm::mat4x4) * 3 + sizeof(glm::vec4) * 2, sizeof(glm::vec4));

			return true;
		}

		void Draw(const glm::mat4x4& _matModel, const ph::BasicModelRef _model, eRenderMode _mode = ph::eRenderModeFill)
		{
			transUBO->WriteData(&_matModel, sizeof(glm::mat4x4) * 2, sizeof(glm::mat4x4));
			for (auto & mesh : _model->meshes)
			{
				mesh.vao->Bind();
				this->diffuseTexSlot->BindTexture(_model->materials[mesh.material].texDiffuse.get() );
				this->specTexSlot->BindTexture(_model->materials[mesh.material].texHighlight.get());
				this->ambientTexSlot->BindTexture(_model->materials[mesh.material].texAmbient.get());
				struct {
					glm::vec4 Ka, Kd, Ks;
					float shiness;
				} c ;
				memcpy(&c.Ka, &_model->materials[mesh.material].ambient, sizeof(glm::vec4));
				memcpy(&c.Kd, &_model->materials[mesh.material].diffuse, sizeof(glm::vec4));
				memcpy(&c.Ks, &_model->materials[mesh.material].specular, sizeof(glm::vec4));
				c.shiness = _model->materials[mesh.material].shiness;
				transUBO->WriteData(&c, sizeof(glm::mat4x4) * 3 + sizeof(glm::vec4) * 3, sizeof(c));

				GLuint modes[] = {
					GL_POINT,
					GL_FILL,
					GL_LINE,
				};
				glPolygonMode(GL_FRONT_AND_BACK, modes[_mode]);
				glDrawElements( GL_TRIANGLES, mesh.nElement, GL_UNSIGNED_INT, 0 );
			}
		}

		void End()
		{

		}

		Basic3DRender() {}
		~Basic3DRender() {}
	};
}




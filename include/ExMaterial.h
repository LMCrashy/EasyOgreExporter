////////////////////////////////////////////////////////////////////////////////
// ExMaterial.h
// Author   : Bastien BOURINEAU
// Start Date : January 21, 2012
////////////////////////////////////////////////////////////////////////////////
/*********************************************************************************
*                                                                                *
*   This program is free software; you can redistribute it and/or modify         *
*   it under the terms of the GNU Lesser General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or            *
*   (at your option) any later version.                                          *
*                                                                                *
**********************************************************************************/
////////////////////////////////////////////////////////////////////////////////
// Port to 3D Studio Max - Modified original version
// Author	      : Doug Perkowski - OC3 Entertainment, Inc.
// From work of : Francesco Giordana
// Start Date   : December 10th, 2007
////////////////////////////////////////////////////////////////////////////////

#ifndef _MATERIAL_H
#define _MATERIAL_H

#include "ExPrerequisites.h"
#include "ExShader.h"

namespace EasyOgreExporter
{
  typedef enum {MT_SURFACE_SHADER, MT_LAMBERT, MT_PHONG, MT_BLINN, MT_CGFX, MT_FACETED, MT_METAL} MaterialType;
	typedef enum {TOT_REPLACE, TOT_MODULATE, TOT_ADD, TOT_ALPHABLEND, TOT_MANUALBLEND} TexOpType;
	typedef enum {TAM_CLAMP, TAM_BORDER, TAM_WRAP, TAM_MIRROR} TexAddressMode;

  class MatProc : public PropertyEnum
  {
    public:
    MatProc();

    virtual bool Proc(IGameProperty * prop);
  };

	class Texture
	{
	public:
		//constructor
		Texture()
    {
			scale_u = scale_v = 1;
			scroll_u = scroll_v = 0;
      scroll_s_u = scroll_s_v = 0;
      rot = rot_s = 0;

			am_u = am_v = TAM_CLAMP;
      type = 0;
      uvsetIndex = 0;
      animRate = 1;

			// Most textures like normal, specular, bump, etc. can't just
			// be summed into the diffuse channel and need
			bCreateTextureUnit = false;
      bReflect = false;
      bHasAlphaChannel = false;
      hasMask = false;
      fAmount = 1.0;
		}
		//destructor
		~Texture(){};
	
		//public members
		std::vector<std::string> filename;
		std::vector<std::string> absFilename;
		TexOpType opType;
		int uvsetIndex;
		bool bCreateTextureUnit;
    bool bReflect;
    bool bHasAlphaChannel;
    bool hasMask;
    float fAmount;
		TexAddressMode am_u, am_v;
		double scale_u, scale_v;
		double scroll_u, scroll_v;
    double scroll_s_u, scroll_s_v;
		double rot;
    double rot_s;
    int animRate;
    int type;
	};


	/***** Class ExMaterial *****/
	class ExMaterial
	{
	public:
    IGameMaterial* m_GameMaterial;
		std::string m_name;
		MaterialType m_type;
		Point4 m_ambient, m_diffuse, m_specular, m_emissive;
    float m_shininess;
    float m_softness;
    float m_opacity;
    float m_reflectivity;
    float m_normalMul;
		bool m_lightingOff;
		bool m_isTransparent;
		bool m_isTextured;
    bool m_isTwoSided;
    bool m_isWire;
    bool m_hasAlpha;
    bool m_bPreMultipliedAlpha;
    bool m_hasDiffuseMap;
    bool m_hasAmbientMap;
    bool m_ambientLocked;
    bool m_hasSpecularMap;
    bool m_hasReflectionMap;
    bool m_hasBumpMap;
		bool m_AmbIsVertColor;
    bool m_DiffIsVertColor;

    std::vector<Texture> m_textures;
  private:
    ExOgreConverter* m_converter;
    unsigned int texUnitId;
  protected:

  public:
		//constructor
		ExMaterial(ExOgreConverter* converter, IGameMaterial* pGameMaterial, std::string prefix);

		//destructor
		~ExMaterial();

		//get material name
		std::string& getName();

		//clear material data
		void clear();

		//load material data
		bool load();

		//write material data to Ogre material script
		bool writeOgreScript(std::ofstream &outMaterial, ExShader* vsAmbShader, ExShader* fpAmbShader, ExShader* vsLightShader, ExShader* fpLightShader);

    std::string getShaderName(ExShader::ShaderType type, std::string prefix);
  private:
    void loadManualTexture(IGameProperty* prop, int type, float amount);
    void loadTextureUV(IGameTextureMap* pGameTexture, Texture &tex);
    void loadArchitectureMaterial(IGameMaterial* pGameMaterial);
    void loadArchAndDesignMaterial(IGameMaterial* pGameMaterial);
    void loadStandardMaterial(IGameMaterial* pGameMaterial);
    void writeMaterialTechnique(std::ofstream &outMaterial, int lod, ExShader* vsAmbShader, ExShader* fpAmbShader, ExShader* vsLightShader, ExShader* fpLightShader);
    void writeMaterialPass(std::ofstream &outMaterial, int lod, ExShader* vsShader, ExShader* fpShader, ExShader::ShaderPass pass);
		bool exportColor(Point4& color, IGameProperty* pGameProperty);
    bool exportSpecular(IGameMaterial* pGameMaterial);
    std::string getMaterialName(std::string prefix);
  protected:
	};

};	//end of namespace

#endif

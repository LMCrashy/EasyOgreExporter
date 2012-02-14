////////////////////////////////////////////////////////////////////////////////
// ExScene.cpp
// Author     : Bastien Bourineau
// Start Date : Junary 11th, 2012
// Copyright  : Copyright (c) 2011 OpenSpace3D
////////////////////////////////////////////////////////////////////////////////
/*********************************************************************************
*                                                                                *
*   This program is free software; you can redistribute it and/or modify         *
*   it under the terms of the GNU Lesser General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or            *
*   (at your option) any later version.                                          *
*                                                                                *
**********************************************************************************/
#include "ExScene.h"
#include "EasyOgreExporterLog.h"
#include "ExMaterial.h"
#include "ExTools.h"
#include "decomp.h"

namespace EasyOgreExporter
{
	// constructor
	ExScene::ExScene(ExOgreConverter* converter, ParamList &params)
	{
		id_counter = 0;
    m_converter = converter;
    mParams = params;

    //TODO test directories ?
    scenePath = makeOutputPath(mParams.outputDir, "", mParams.sceneFilename, "scene");

    initXmlDocument();
	}

	// destructor
	ExScene::~ExScene()
	{
    if (xmlDoc != 0)
      delete(xmlDoc);
	}

  void ExScene::initXmlDocument()
  {
    xmlDoc = new TiXmlDocument();
    sceneElement = new TiXmlElement("scene");
    xmlDoc->LinkEndChild(sceneElement);

    //TODO Ogre version, App Name, units
    sceneElement->SetAttribute("upAxis", mParams.yUpAxis ? "y" : "z");
    sceneElement->SetAttribute("unitsPerMeter", "1");
    sceneElement->SetAttribute("unitType", "meters");
    sceneElement->SetAttribute("formatVersion", "1.0");
		sceneElement->SetAttribute("minOgreVersion", "1.8");
		sceneElement->SetAttribute("author", "EasyOgreExporter");

    //Environment setting
    TiXmlElement* envElement = new TiXmlElement("environment");
    sceneElement->LinkEndChild(envElement);

    Point3 ambColor = GetCOREInterface()->GetAmbient(0, FOREVER);
    TiXmlElement* ambElement = new TiXmlElement("colourAmbient");
    ambElement->SetDoubleAttribute("r", ambColor.x);
	  ambElement->SetDoubleAttribute("g", ambColor.y);
	  ambElement->SetDoubleAttribute("b", ambColor.z);
    envElement->LinkEndChild(ambElement);
    
    Point3 bgColor = GetCOREInterface()->GetBackGround(0, FOREVER);
    TiXmlElement* bgElement = new TiXmlElement("colourBackground");
    bgElement->SetDoubleAttribute("r", bgColor.x);
	  bgElement->SetDoubleAttribute("g", bgColor.y);
	  bgElement->SetDoubleAttribute("b", bgColor.z);
    envElement->LinkEndChild(bgElement);

		nodesElement = new TiXmlElement("nodes");
		sceneElement->LinkEndChild(nodesElement);
  }

  std::string ExScene::getBoolString(bool value)
  {
    return value ? std::string("true") : std::string("false");
  }

	std::string ExScene::getLightTypeString(ExOgreLightType type)
	{
		switch(type)
		{
		case OGRE_LIGHT_POINT:
			return std::string("point");
		case OGRE_LIGHT_DIRECTIONAL:
			return std::string("directional");
		case OGRE_LIGHT_SPOT:
			return std::string("spot");
		case OGRE_LIGHT_RADPOINT:
			return std::string("radpoint");
		}
		EasyOgreExporterLog("Invalid light type detected. Using point light as default.\n");
		return("point");
	}

	TiXmlElement* ExScene::writeNodeData(TiXmlElement* parent, IGameNode* pGameNode, IGameObject::ObjectTypes type)
	{
		if(!parent)
      parent = nodesElement;

    INode* maxnode = pGameNode->GetMaxNode();
    if (maxnode->GetParentNode())
      maxnode->GetParentNode()->EvalWorldState(0);
    
    // get the relative transform
    Matrix3 nodeTM = GetRelativeMatrix(maxnode, 0, mParams.yUpAxis);

    AffineParts ap;
    decomp_affine(nodeTM, &ap);

    Point3 trans = ap.t * mParams.lum;
    Point3 scale = ap.k;
    Quat rot = ap.q;

    if((type == IGameObject::IGAME_CAMERA) && mParams.yUpAxis)
    {
      // Now rotate around the X Axis PI/2
      Quat zRev = RotateXMatrix(PI/2);
      rot = rot / zRev;
    }
    
    if((type == IGameObject::IGAME_LIGHT) && mParams.yUpAxis)
    {
      // Now rotate around the X Axis -PI/2
      Quat zRev = RotateXMatrix(-PI/2);
      rot = rot / zRev;
    }

    // Notice that in Max we flip the w-component of the quaternion;
    rot.w = -rot.w;

		TiXmlElement* pNodeElement = new TiXmlElement("node");
		pNodeElement->SetAttribute("name", pGameNode->GetName());
		pNodeElement->SetAttribute("id", id_counter);
		pNodeElement->SetAttribute("isTarget", "false");
		parent->LinkEndChild(pNodeElement);

		TiXmlElement* pPositionElement = new TiXmlElement("position");
    pPositionElement->SetDoubleAttribute("x", trans.x);
		pPositionElement->SetDoubleAttribute("y", trans.y);
		pPositionElement->SetDoubleAttribute("z", trans.z);
		pNodeElement->LinkEndChild(pPositionElement);

		TiXmlElement* pRotationElement = new TiXmlElement("rotation");
		pRotationElement->SetDoubleAttribute("qx", rot.x);
		pRotationElement->SetDoubleAttribute("qy", rot.y);
		pRotationElement->SetDoubleAttribute("qz", rot.z);
		// Notice that in Max we flip the w-component of the quaternion;
		pRotationElement->SetDoubleAttribute("qw", rot.w);
		pNodeElement->LinkEndChild(pRotationElement);

		TiXmlElement* pScaleElement = new TiXmlElement("scale");
		pScaleElement->SetDoubleAttribute("x", scale.x);
		pScaleElement->SetDoubleAttribute("y", scale.y);
		pScaleElement->SetDoubleAttribute("z", scale.z);
		pNodeElement->LinkEndChild(pScaleElement);

    //node animations
    Interval animRange = GetCOREInterface()->GetAnimRange();
    IGameControl* nodeControl = pGameNode->GetIGameControl();

    //anim layer or motion mixer ?
    //IAnimLayerControlManager* layerManager = static_cast<IAnimLayerControlManager*>(GetCOREInterface(IANIMLAYERCONTROLMANAGER_INTERFACE));
    if(nodeControl->IsAnimated(IGAME_POS) || nodeControl->IsAnimated(IGAME_ROT) || nodeControl->IsAnimated(IGAME_SCALE))
    {
      std::vector<int> animKeys;

      //get the keys from max STD controllers
      Control* controlPos = maxnode->GetTMController()->GetPositionController();
	    IKeyControl* ikcPos = GetKeyControlInterface(controlPos);
      if(ikcPos)
      {
        for (int keyPos = 0; keyPos < ikcPos->GetNumKeys(); keyPos++)
        {
          IKey nkey;
          ikcPos->GetKey(keyPos, &nkey);
          if((nkey.time >= animRange.Start()) && (nkey.time <= animRange.End()))
          {
            animKeys.push_back(nkey.time);
          }
        }
      }

	    Control* controlRot = maxnode->GetTMController()->GetRotationController();
	    IKeyControl* ikcRot = GetKeyControlInterface(controlRot);
      if(ikcRot)
      {
        for (int keyPos = 0; keyPos < ikcRot->GetNumKeys(); keyPos++)
        {
          IKey nkey;
          ikcRot->GetKey(keyPos, &nkey);
          if((nkey.time >= animRange.Start()) && (nkey.time <= animRange.End()))
          {
            animKeys.push_back(nkey.time);
          }
        }
      }

	    Control* controlScale = maxnode->GetTMController()->GetScaleController();
	    IKeyControl* ikcScale = GetKeyControlInterface(controlScale);
      if(ikcScale)
      {
        for (int keyPos = 0; keyPos < ikcScale->GetNumKeys(); keyPos++)
        {
          IKey nkey;
          ikcScale->GetKey(keyPos, &nkey);
          if((nkey.time >= animRange.Start()) && (nkey.time <= animRange.End()))
          {
            animKeys.push_back(nkey.time);
          }
        }
      }

      //if you prefer IGame
      /*IGameKeyTab poskeys;
      nodeControl->GetBezierKeys(poskeys, IGAME_POS);
      for (int keyPos = 0; keyPos > poskeys.Count(); keyPos++)
      {
        IGameKey nkey = poskeys[keyPos];
        if((nkey.t >= animRange.Start()) && (nkey.t <= animRange.End()))
        {
          animKeys.push_back(nkey.t);
        }
      }*/

      //sort and remove duplicated entries
      std::sort(animKeys.begin(), animKeys.end());
      animKeys.erase(std::unique(animKeys.begin(), animKeys.end()), animKeys.end());

      if(animKeys.size() > 0)
      {
        TimeValue length = animRange.End() - animRange.Start();
        float ogreAnimLength = (static_cast<float>(length) / static_cast<float>(GetTicksPerFrame())) / GetFrameRate();

        //Add the default animation and the track
        TiXmlElement* pAnimsElement = new TiXmlElement("animations");
        pNodeElement->LinkEndChild(pAnimsElement);

        TiXmlElement* pAnimElement = new TiXmlElement("animation");
        pAnimElement->SetAttribute("name", "default");
        pAnimElement->SetAttribute("enable", "true");
        pAnimElement->SetAttribute("loop", "false");
        pAnimElement->SetAttribute("interpolationMode", "linear");
        pAnimElement->SetAttribute("rotationInterpolationMode", "linear");
        pAnimElement->SetDoubleAttribute("length", ogreAnimLength);
        pAnimsElement->LinkEndChild(pAnimElement);
        
        for(int i = 0; i < animKeys.size(); i++)
        {
          if (maxnode->GetParentNode())
            maxnode->GetParentNode()->EvalWorldState(0);
          
          // get the relative transform
          Matrix3 keyTM = GetRelativeMatrix(maxnode, animKeys[i], mParams.yUpAxis);

          float ogreTime = (static_cast<float>(animKeys[i] - animRange.Start()) / static_cast<float>(GetTicksPerFrame())) / GetFrameRate();

          AffineParts apKey;
          decomp_affine(keyTM, &apKey);

          Point3 trans = apKey.t * mParams.lum;
          Point3 scale = apKey.k;
          Quat rot = apKey.q;

          if((type == IGameObject::IGAME_CAMERA) && mParams.yUpAxis)
          {
            // Now rotate around the X Axis PI/2
            Quat zRev = RotateXMatrix(PI/2);
            rot = rot / zRev;
          }
          
          if((type == IGameObject::IGAME_LIGHT) && mParams.yUpAxis)
          {
            // Now rotate around the X Axis -PI/2
            Quat zRev = RotateXMatrix(-PI/2);
            rot = rot / zRev;
          }

          // Notice that in Max we flip the w-component of the quaternion;
          rot.w = -rot.w;

          TiXmlElement* pKeyElement = new TiXmlElement("keyframe");
          pKeyElement->SetDoubleAttribute("time", ogreTime);
          pAnimElement->LinkEndChild(pKeyElement);

          TiXmlElement* pKeyTransElement = new TiXmlElement("translation");
          pKeyTransElement->SetDoubleAttribute("x", trans.x);
          pKeyTransElement->SetDoubleAttribute("y", trans.y);
          pKeyTransElement->SetDoubleAttribute("z", trans.z);
          pKeyElement->LinkEndChild(pKeyTransElement);

          TiXmlElement* pKeyRotElement = new TiXmlElement("rotation");
          pKeyRotElement->SetDoubleAttribute("qx", rot.x);
          pKeyRotElement->SetDoubleAttribute("qy", rot.y);
          pKeyRotElement->SetDoubleAttribute("qz", rot.z);
          pKeyRotElement->SetDoubleAttribute("qw", rot.w);
          pKeyElement->LinkEndChild(pKeyRotElement);

          TiXmlElement* pKeyScaleElement = new TiXmlElement("scale");
          pKeyScaleElement->SetDoubleAttribute("x", scale.x);
          pKeyScaleElement->SetDoubleAttribute("y", scale.y);
          pKeyScaleElement->SetDoubleAttribute("z", scale.z);
          pKeyElement->LinkEndChild(pKeyScaleElement);
        }
      }
    }

    id_counter++;
		return pNodeElement;
	}

	TiXmlElement* ExScene::writeEntityData(TiXmlElement* parent, IGameMesh* pGameMesh)
	{
		if(!parent)
      return 0;
    
    if (!pGameMesh->IsEntitySupported())
    {
      EasyOgreExporterLog("Unsupported mesh type. Failed to export.\n");
      return 0;
    }

		TiXmlElement* pEntityElement = new TiXmlElement("entity");
    pEntityElement->SetAttribute("name", parent->Attribute("name"));
		pEntityElement->SetAttribute("id", id_counter);

    std::string meshPath = std::string(parent->Attribute("name")) + ".mesh";
    pEntityElement->SetAttribute("meshFile", meshPath.c_str());
    pEntityElement->SetAttribute("castShadows", getBoolString(pGameMesh->CastShadows()).c_str());
    parent->LinkEndChild(pEntityElement);

    TiXmlElement* pSubEntities = new TiXmlElement("subentities");
    pEntityElement->LinkEndChild(pSubEntities);

    Tab<int> subEntities = pGameMesh->GetActiveMatIDs();
    for (int i = 0; i < subEntities.Count(); i++)
    {
      Tab<FaceEx*> faces = pGameMesh->GetFacesFromMatID(subEntities[i]);
      if (faces.Count() > 0)
      {
        IGameMaterial* pGameMaterial = pGameMesh->GetMaterialFromFace(faces[0]);
        Material* mat = m_converter->getMaterialSet()->getMaterial(pGameMaterial);

        TiXmlElement* pSubElement = new TiXmlElement("subentity");
        pSubElement->SetAttribute("index", i);
        pSubElement->SetAttribute("materialName", mat->name().c_str());
        pSubEntities->LinkEndChild(pSubElement);
      }
    }

    id_counter++;
		return pEntityElement;
	}

  TiXmlElement* ExScene::writeCameraData(TiXmlElement* parent, IGameCamera* pGameCamera)
  {
		if(!parent)
      return 0;

    if (!pGameCamera->IsEntitySupported())
    {
      EasyOgreExporterLog("Unsupported camera type. Failed to export.\n");
      return 0;
    }

    TiXmlElement* pCameraElement = new TiXmlElement("camera");
		pCameraElement->SetAttribute("name", parent->Attribute("name"));
		pCameraElement->SetAttribute("id", id_counter);

    //default 45�
    float camFov = 0.785398163f;
    IGameProperty* pGameProperty = pGameCamera->GetCameraFOV();
    if(pGameProperty)
    {
      pGameProperty->GetPropertyValue(camFov);
    }
    pCameraElement->SetDoubleAttribute("fov", camFov);
    parent->LinkEndChild(pCameraElement);

    float neerClip = 0.01f;
    pGameProperty = pGameCamera->GetCameraNearClip ();
    if(pGameProperty)
    {
      pGameProperty->GetPropertyValue(neerClip);
    }
    neerClip = neerClip * mParams.lum;

    float farClip = 1000.0f;
    pGameProperty = pGameCamera->GetCameraFarClip();
    if(pGameProperty)
    {
      pGameProperty->GetPropertyValue(farClip);
    }
    farClip = farClip * mParams.lum;

		TiXmlElement* pClippingElement = new TiXmlElement("clipping");
		pClippingElement->SetDoubleAttribute("near", neerClip);
		pClippingElement->SetDoubleAttribute("far", farClip);
		pCameraElement->LinkEndChild(pClippingElement);

    id_counter++;
		return pCameraElement;
  }

  TiXmlElement* ExScene::writeLightData(TiXmlElement* parent, IGameLight* pGameLight)
  {
		if(!parent)
      return 0;
    
    if (!pGameLight->IsEntitySupported())
    {
      EasyOgreExporterLog("Unsupported light type. Failed to export.\n");
      return 0;
    }

    std::string lightClass = pGameLight->GetClassName();
    if(lightClass == "Missing Light")
    {
      EasyOgreExporterLog("Unsupported light type. Failed to export.\n");
      return 0;
    }

    ExOgreLightType lightType = OGRE_LIGHT_DIRECTIONAL;
    switch(pGameLight->GetLightType())
    {
      case IGameLight::IGAME_OMNI:
        lightType = OGRE_LIGHT_POINT;
        break;
      case IGameLight::IGAME_DIR:
        lightType = OGRE_LIGHT_DIRECTIONAL;
        break;
      case IGameLight::IGAME_FSPOT:
        lightType = OGRE_LIGHT_SPOT;
        break;
      // No support for targeted lights at the moment, but
      // We can still export them facing the initial direction.
      case IGameLight::IGAME_TSPOT:
        lightType = OGRE_LIGHT_SPOT;
        break;
      case IGameLight::IGAME_TDIR:
        lightType = OGRE_LIGHT_DIRECTIONAL;
        break;
      case IGameLight::IGAME_UNKNOWN:
      default:
        EasyOgreExporterLog("Unsupported light type. Failed to export.\n");
        return 0;
    }

    TiXmlElement* pLightElement = new TiXmlElement("light");
		pLightElement->SetAttribute("name", parent->Attribute("name"));
		pLightElement->SetAttribute("id", id_counter);
		pLightElement->SetAttribute("type", getLightTypeString(lightType).c_str());
		pLightElement->SetAttribute("castShadows", getBoolString(pGameLight->CastShadows()).c_str());
		parent->LinkEndChild(pLightElement);

    IGameProperty* pGameProperty = pGameLight->GetLightColor();
    float propertyValue;
    Point3 lightColor;
    if(pGameProperty)
    {
      pGameProperty->GetPropertyValue(lightColor);
    }

		TiXmlElement* pColourElement = new TiXmlElement("colourDiffuse");
		pColourElement->SetDoubleAttribute("r", lightColor.x);
		pColourElement->SetDoubleAttribute("g", lightColor.y);
		pColourElement->SetDoubleAttribute("b", lightColor.z);
		pLightElement->LinkEndChild(pColourElement);
    
    TiXmlElement* pSpecColourElement = new TiXmlElement("colourSpecular");
		pSpecColourElement->SetDoubleAttribute("r", lightColor.x);
		pSpecColourElement->SetDoubleAttribute("g", lightColor.y);
		pSpecColourElement->SetDoubleAttribute("b", lightColor.z);
		pLightElement->LinkEndChild(pSpecColourElement);
		
		if(lightType == OGRE_LIGHT_POINT)
		{
      float attRange = 10.0f;
      float attConst = 1.0f;
      float attLinear = 0.0f;
      float attQuad = 0.0f;

      // Max lets attenuation start at a specific point.  Ogre seems to 
      // require that attenuation begins immediately.  We use the
      // Attenuation end as the range.
      pGameProperty = pGameLight->GetLightAttenEnd();
      if(pGameProperty)
      {
        pGameProperty->GetPropertyValue(attRange);	
      }
      attRange = attRange * mParams.lum;

      // Inverse decay
      if(pGameLight->GetLightDecayType() == 1)
      {
        attLinear = 1.0f;
      }
      // Inverse Square decay
      else if(pGameLight->GetLightDecayType() == 2)
      {
        attQuad = 1.0f;
      }

			TiXmlElement* pAttenuationElement = new TiXmlElement("lightAttenuation");
			pAttenuationElement->SetDoubleAttribute("range", attRange);
			pAttenuationElement->SetDoubleAttribute("constant", attConst);
			pAttenuationElement->SetDoubleAttribute("linear", attLinear);
			pAttenuationElement->SetDoubleAttribute("quadratic", attQuad);
			pLightElement->LinkEndChild(pAttenuationElement);
		}

		if(lightType == OGRE_LIGHT_SPOT)
		{
      float rangeInner = 0.0f;
      float rangeFalloff  = 1.0f;
      float rangeOuter = 1.0f;
      pGameProperty = pGameLight->GetLightFallOff();
      if(pGameProperty)
      {
        // Max seems to use GetLightFallOff to get the outer angle.  
        // This doesn't leave anything for falloff. 
        pGameProperty->GetPropertyValue(propertyValue);	
        rangeOuter = propertyValue * PI / 180.0f;
      }
      pGameProperty = pGameLight->GetLightHotSpot();
      if(pGameProperty)
      {
        pGameProperty->GetPropertyValue(propertyValue);	
        rangeInner = propertyValue * PI / 180.0f;
      }

			TiXmlElement* pAttenuationElement = new TiXmlElement("lightRange");
			pAttenuationElement->SetDoubleAttribute("inner", rangeInner);
			pAttenuationElement->SetDoubleAttribute("outer", rangeOuter);
			pAttenuationElement->SetDoubleAttribute("falloff", rangeFalloff);
			pLightElement->LinkEndChild(pAttenuationElement);
		}

    id_counter++;
		return pLightElement;
  }

	bool ExScene::writeSceneFile()
	{    
		return xmlDoc->SaveFile(scenePath.c_str());
	}

}; //end of namespace

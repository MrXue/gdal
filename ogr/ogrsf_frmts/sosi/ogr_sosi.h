/******************************************************************************
 *
 * Project:  SOSI Translator
 * Purpose:  Implements OGRSOSIDriver.
 * Author:   Thomas Hirsch, <thomas.hirsch statkart no>
 *
 ******************************************************************************
 * Copyright (c) 2010, Thomas Hirsch
 * Copyright (c) 2010, Even Rouault <even dot rouault at spatialys.com>
 *
 * SPDX-License-Identifier: MIT
 ****************************************************************************/

#ifndef OGR_SOSI_H_INCLUDED
#define OGR_SOSI_H_INCLUDED

#include "ogrsf_frmts.h"
#include "fyba.h"
#include <map>

// Note: WRITE_SUPPORT not defined, since this is only partially implemented

/* interpolation of arcs(BUEP) creates # points for a full circle */
#define ARC_INTERPOLATION_FULL_CIRCLE 36.0

typedef std::map<std::string, std::string> S2S;
typedef std::map<std::string, unsigned int> S2I;

void RegisterOGRSOSI();

class ORGSOSILayer;      /* defined below */
class OGRSOSIDataSource; /* defined below */

/************************************************************************
 *                           OGRSOSILayer                               *
 * OGRSOSILayer reports all the OGR Features generated by the data      *
 * source, in an orderly fashion.                                       *
 ************************************************************************/

class OGRSOSILayer final : public OGRLayer
{
    int nNextFID;

    OGRSOSIDataSource *poParent; /* used to call methods from data source */
    LC_FILADM *poFileadm; /* ResetReading needs to refer to the file struct */
    OGRFeatureDefn *poFeatureDefn; /* the common definition of all features
                                      returned by this layer  */
    S2I *poHeaderDefn;

    LC_SNR_ADM oSnradm;
    LC_BGR oNextSerial; /* used by FYBA to iterate through features */
    LC_BGR *poNextSerial;

  public:
    OGRSOSILayer(OGRSOSIDataSource *poPar, OGRFeatureDefn *poFeatDefn,
                 LC_FILADM *poFil, S2I *poHeadDefn);
    ~OGRSOSILayer();

    void ResetReading() override;
    OGRFeature *GetNextFeature() override;
    OGRFeatureDefn *GetLayerDefn() override;
#ifdef WRITE_SUPPORT
    OGRErr CreateField(OGRFieldDefn *poField, int bApproxOK = TRUE) override;
    OGRErr ICreateFeature(OGRFeature *poFeature) override;
#endif
    int TestCapability(const char *) override;
};

/************************************************************************
 *                           OGRSOSIDataSource                          *
 * OGRSOSIDataSource reads a SOSI file, prebuilds the features, and     *
 * creates one OGRSOSILayer per geometry type                           *
 ************************************************************************/
class OGRSOSIDataSource final : public GDALDataset
{
    OGRSOSILayer **papoLayers;
    int nLayers;

#define MODE_READING 0
#define MODE_WRITING 1
    int nMode;

    void buildOGRPoint(long nSerial);
    void buildOGRLineString(int nNumCoo, long nSerial);
    void buildOGRMultiPoint(int nNumCoo, long nSerial);
    void buildOGRLineStringFromArc(long nSerial);

  public:
    OGRSpatialReference *poSRS;
    const char *pszEncoding;
    unsigned int nNumFeatures;
    OGRGeometry **papoBuiltGeometries; /* OGRSOSIDataSource prebuilds some
                                        * features upon opening, te be used by
                                        * the more complex geometries later. */
    // FYBA specific
    LC_BASEADM *poBaseadm;
    LC_FILADM *poFileadm;

    S2I *poPolyHeaders; /* Contain the header definitions of the four feature
                           layers */
    S2I *poPointHeaders;
    S2I *poCurveHeaders;
    S2I *poTextHeaders;

    OGRSOSIDataSource();
    ~OGRSOSIDataSource();

    int Open(const char *pszFilename, int bUpdate);
#ifdef WRITE_SUPPORT
    int Create(const char *pszFilename);
#endif
    int GetLayerCount() override
    {
        return nLayers;
    }

    OGRLayer *GetLayer(int) override;
#ifdef WRITE_SUPPORT
    OGRLayer *ICreateLayer(const char *pszName,
                           const OGRSpatialReference *poSpatialRef = NULL,
                           OGRwkbGeometryType eGType = wkbUnknown,
                           char **papszOptions = NULL) override;
#endif
};

/************************************************************************
 *                           OGRSOSIDataTypes                           *
 * OGRSOSIDataTypes provides the correct data types for some of the     *
 * most common SOSI elements.                                           *
 ************************************************************************/

class OGRSOSISimpleDataType
{
    CPLString osName;
    OGRFieldType nType;

  public:
    OGRSOSISimpleDataType();
    OGRSOSISimpleDataType(const char *pszName, OGRFieldType nType);
    ~OGRSOSISimpleDataType();

    void setType(const char *pszName, OGRFieldType nType);

    const char *GetName() const
    {
        return osName.c_str();
    }

    OGRFieldType GetType() const
    {
        return nType;
    }
};

class OGRSOSIDataType
{
    OGRSOSISimpleDataType *poElements = nullptr;
    int nElementCount = 0;

    OGRSOSIDataType &operator=(const OGRSOSIDataType &) = delete;

  public:
    explicit OGRSOSIDataType(int nSize);

    OGRSOSIDataType(const OGRSOSIDataType &oSrc)
        : poElements(nullptr), nElementCount(oSrc.nElementCount)
    {
        poElements = new OGRSOSISimpleDataType[nElementCount];
        for (int i = 0; i < nElementCount; i++)
            poElements[i] = oSrc.poElements[i];
    }

    OGRSOSIDataType(OGRSOSIDataType &&oSrc) noexcept
        : poElements(oSrc.poElements), nElementCount(oSrc.nElementCount)
    {
        oSrc.poElements = nullptr;
        oSrc.nElementCount = 0;
    }

    ~OGRSOSIDataType();

    void setElement(int nIndex, const char *name, OGRFieldType type);

    OGRSOSISimpleDataType *getElements()
    {
        return poElements;
    }

    int getElementCount()
    {
        return nElementCount;
    }
};

typedef std::map<CPLString, OGRSOSIDataType> C2F;

void SOSIInitTypes();
void SOSICleanupTypes();
OGRSOSIDataType *SOSIGetType(const CPLString &name);
int SOSITypeToInt(const char *value);
double SOSITypeToReal(const char *value);
void SOSITypeToDate(const char *value, int *date);
void SOSITypeToDateTime(const char *value, int *date);

#endif /* OGR_SOSI_H_INCLUDED */

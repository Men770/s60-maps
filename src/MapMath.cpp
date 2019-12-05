/*
 * MapMath.cpp
 *
 *  Created on: 12.08.2019
 *      Author: user
 */

// ToDo: Define constants
// Formulas source: https://wiki.openstreetmap.org/wiki/Zoom_levels

#include "MapMath.h"
#include <e32math.h>

void MapMath::PixelsToMeters(const TReal64 &aLatitude, /*TUint8*/ TInt aZoom, TUint aPixels,
			TReal32 &aHorizontalDistance, TReal32 &aVerticalDistance)
	{
	TReal c;
	Math::Cos(c, aLatitude * KDegToRad);
	//TReal p;
	//Math::Pow(p, 2, aZoom + 8);
	//TInt p = 2 ** (aZoom + 8);
	TReal p;
	Math::Pow(p, 2, aZoom + 8);
	aHorizontalDistance =	aPixels * 40075016.686 / p;	
	aVerticalDistance =		aPixels * 40075016.686 * c / p;
	}

void MapMath::PixelsToDegrees(const TReal64 &aLatitude, /*TUint8*/ TInt aZoom, TUint aPixels,
			/*TReal32*/ TReal64 &aHorizontalAngle, /*TReal32*/ TReal64 &aVerticalAngle)
	{
	TReal32 horizontalDistance;
	TReal32 verticalDistance;
	PixelsToMeters(aLatitude, aZoom, aPixels, horizontalDistance, verticalDistance);
	aHorizontalAngle =	horizontalDistance / (40075696.0 / 360.0);
	aVerticalAngle =	verticalDistance   / (40075696.0 / 360.0);
	}

TTile MapMath::GeoCoordsToTile(const TCoordinate &aCoord, /*TUint8*/ TInt aZoom)
	{
	TTileReal tileReal = GeoCoordsToTileReal(aCoord, aZoom);
	TTile tile;
	tile.iX = (TUint) tileReal.iX;
	tile.iY = (TUint) tileReal.iY;
	tile.iZ = tileReal.iZ;
	return tile;
	}

TTileReal MapMath::GeoCoordsToTileReal(const TCoordinate &aCoord, /*TUint8*/ TInt aZoom)
	{
	// https://wiki.openstreetmap.org/wiki/Slippy_map_tilenames#Lon..2Flat._to_tile_numbers_2
	
	TTileReal tile;
	tile.iZ = aZoom;
	
	// (int)(floor((lon + 180.0) / 360.0 * (1 << z))); 
	tile.iX = (aCoord.Longitude() + 180.0) / 360.0 * (1 << aZoom);
	
    //double latrad = lat * M_PI/180.0;
	//return (int)(floor((1.0 - asinh(tan(latrad)) / M_PI) / 2.0 * (1 << z)));
	//TReal latrad = aCoord.Latitude() * KPi;
	//tile.iY = (TUint) (1.0 - asinh(tan(latrad)) / KPi) / 2.0 * (1 << aZoom));
	// (1 - math.log(math.tan(math.radians(lat)) + 1 / math.cos(math.radians(lat))) / math.pi) / 2 * 2 ** zoom
	TReal latRad = aCoord.Latitude() * KDegToRad;
	TReal t;
	Math::Tan(t, latRad);
	TReal c;
	Math::Cos(c, latRad);
	TReal l;
	Math::Ln(l, t + 1.0 / c);
	
	TReal p;
	Math::Pow(p, 2, aZoom);
	tile.iY = (1.0 - l / KPi) / 2 * /*2 ** aZoom*/ p;
	
	return tile;
	}

TCoordinate MapMath::TileToGeoCoords(const TTileReal &aTile, /*TUint8*/ TInt aZoom)
	{
	/*double tilex2long(int x, int z) 
	{
		return x / (double)(1 << z) * 360.0 - 180;
	}

	double tiley2lat(int y, int z) 
	{
		double n = M_PI - 2.0 * M_PI * y / (double)(1 << z);
		return 180.0 / M_PI * atan(0.5 * (exp(n) - exp(-n)));
	}*/
	
	TCoordinate coord;
	TReal64 lon = aTile.iX / (TReal64)(1 << aZoom) * 360.0 - 180;
	TReal64 n = KPi - 2.0 * KPi * aTile.iY / (TReal64)(1 << aZoom);
	TReal64 pe;
	Math::Exp(pe, n);
	TReal64 ne;
	Math::Exp(ne, -n);
	TReal64 at;
	Math::ATan(at, 0.5 * (pe - ne));
	TReal64 lat = 180.0 / KPi * at;
	coord.SetCoordinate(lat, lon);
	return coord;	
	}

TCoordinate MapMath::TileToGeoCoords(const TTile &aTile, /*TUint8*/ TInt aZoom)
	{
	TTileReal tileReal;
	tileReal.iX = aTile.iX;
	tileReal.iY = aTile.iY;
	tileReal.iZ = aTile.iZ;
	return TileToGeoCoords(tileReal, aZoom);
	}
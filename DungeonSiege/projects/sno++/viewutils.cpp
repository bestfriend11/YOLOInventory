
// This code was lifted 'as is' from SAMPLES/OBJECTS/HELPERS/PTHELP.CPP


#include "MAX.H"



/*--------------------------------------------------------------------*/
// 
// Stole this from scene.cpp
// Probably couldn't hurt to make an API...
//
//


void Text( ViewExp *vpt, TCHAR *str, Point3 &pt )
	{	
	vpt->getGW()->text( &pt, str );	
	}

static void DrawAnAxis( ViewExp *vpt, Point3 axis )
	{
	Point3 v1, v2, v[3];	
	v1 = axis * (float)0.9;
	if ( axis.x != 0.0 || axis.y != 0.0 ) {
		v2 = Point3( axis.y, -axis.x, axis.z ) * (float)0.1;
	} else {
		v2 = Point3( axis.x, axis.z, -axis.y ) * (float)0.1;
		}
	
	v[0] = Point3(0.0,0.0,0.0);
	v[1] = axis;
	vpt->getGW()->polyline( 2, v, NULL, NULL, FALSE, NULL );	
	v[0] = axis;
	v[1] = v1+v2;
	vpt->getGW()->polyline( 2, v, NULL, NULL, FALSE, NULL );
	v[0] = axis;
	v[1] = v1-v2;
	vpt->getGW()->polyline( 2, v, NULL, NULL, FALSE, NULL );
	}

 #define ZFACT (float).005;

void DrawAxis( ViewExp *vpt, const Matrix3 &tm, float length, BOOL sel, BOOL frozen )
	{
	Matrix3 tmn = tm;
	float zoom;
	int limits;	

	// Get width of viewport in world units:  --DS
	zoom = vpt->GetScreenScaleFactor(tmn.GetTrans())*ZFACT;
	
	tmn.Scale( Point3(zoom,zoom,zoom) );
	vpt->getGW()->setTransform( tmn );	

	limits = vpt->getGW()->getRndLimits();
	vpt->getGW()->setRndLimits( limits & ~GW_Z_BUFFER );

	if (sel) {
		vpt->getGW()->setColor( TEXT_COLOR, GetUIColor(COLOR_SELECTION) );
		vpt->getGW()->setColor( LINE_COLOR, GetUIColor(COLOR_SELECTION) );
	} else if (!frozen) {
		vpt->getGW()->setColor( TEXT_COLOR, GetUIColor(COLOR_POINT_AXES) );
		vpt->getGW()->setColor( LINE_COLOR, GetUIColor(COLOR_POINT_AXES) );
		}
	
	Text( vpt, _T("x"), Point3(length,0.0f,0.0f) ); 
	DrawAnAxis( vpt, Point3(length,0.0f,0.0f) );	
	
	Text( vpt, _T("y"), Point3(0.0f,length,0.0f) ); 
	DrawAnAxis( vpt, Point3(0.0f,length,0.0f) );	
	
	Text( vpt, _T("z"), Point3(0.0f,0.0f,length) ); 
	DrawAnAxis( vpt, Point3(0.0f,0.0f,length) );	
	
	vpt->getGW()->setRndLimits( limits );
	}

Box3 GetAxisBox(ViewExp *vpt, const Matrix3 &tm,float length,int resetTM)
	{
	Matrix3 tmn = tm;
	Box3 box;
	float zoom;

	// Get width of viewport in world units:  --DS
	zoom = vpt->GetScreenScaleFactor(tmn.GetTrans())*ZFACT;
	if (zoom==0.0f) zoom = 1.0f;
//	tmn.Scale(Point3(zoom,zoom,zoom));
	length *= zoom;
	if(resetTM)
		tmn.IdentityMatrix();

	box += Point3(0.0f,0.0f,0.0f) * tmn;
	box += Point3(length,0.0f,0.0f) * tmn;
	box += Point3(0.0f,length,0.0f) * tmn;
	box += Point3(0.0f,0.0f,length) * tmn;	
	box += Point3(-length/5.f,0.0f,0.0f) * tmn;
	box += Point3(0.0f,-length/5.f,0.0f) * tmn;
	box += Point3(0.0f,0.0f,-length/5.0f) * tmn;
	box.EnlargeBy(10.0f/zoom);
	return box;
	}


inline void EnlargeRectIPoint3( RECT *rect, IPoint3& pt )
	{
	if ( pt.x < rect->left )   rect->left   = pt.x;
	if ( pt.x > rect->right )  rect->right  = pt.x;
	if ( pt.y < rect->top )    rect->top    = pt.y;
	if ( pt.y > rect->bottom ) rect->bottom = pt.y;
	}

// This is a guess - need to find real w/h.
#define FONT_HEIGHT	10
#define FONT_WIDTH  8	


static void AxisRect( GraphicsWindow *gw, Point3 axis, Rect *rect )
	{
	Point3 v1, v2, v;	
	IPoint3 iv;
	v1 = axis * (float)0.9;
	if ( axis.x != 0.0 || axis.y != 0.0 ) {
		v2 = Point3( axis.y, -axis.x, axis.z ) * (float)0.1;
	} else {
		v2 = Point3( axis.x, axis.z, -axis.y ) * (float)0.1;
		}
	v = axis;
	gw->wTransPoint( &v, &iv );
	EnlargeRectIPoint3( rect, iv);

	iv.x += FONT_WIDTH;
	iv.y -= FONT_HEIGHT;
	EnlargeRectIPoint3( rect, iv);

	v = v1+v2;
	gw->wTransPoint( &v, &iv );
	EnlargeRectIPoint3( rect, iv);
	v = v1-v2;
	gw->wTransPoint( &v, &iv );
	EnlargeRectIPoint3( rect, iv);
	}


void AxisViewportRect(ViewExp *vpt, const Matrix3 &tm, float length, Rect *rect)
	{
	Matrix3 tmn = tm;
	float zoom;
	IPoint3 wpt;
	Point3 pt;
	GraphicsWindow *gw = vpt->getGW();

	// Get width of viewport in world units:  --DS
	zoom = vpt->GetScreenScaleFactor(tmn.GetTrans())*ZFACT;
	
	tmn.Scale( Point3(zoom,zoom,zoom) );
	gw->setTransform( tmn );	
	pt = Point3(0.0f, 0.0f, 0.0f);
	gw->wTransPoint( &pt, &wpt );
	rect->left = rect->right  = wpt.x;
	rect->top  = rect->bottom = wpt.y;

	AxisRect( gw, Point3(length,0.0f,0.0f),rect );	
	AxisRect( gw, Point3(0.0f,length,0.0f),rect );	
	AxisRect( gw, Point3(0.0f,0.0f,length),rect );	

	rect->right  += 2;
	rect->bottom += 2;
	rect->left   -= 2;
	rect->top    -= 2;
	}




#include "export.h"
#include "printutils.h"
#include "OffscreenView.h"
#include "CsgInfo.h"
#include <stdio.h>
#include "polyset.h"
#include "rendersettings.h"

#ifdef ENABLE_CGAL
#include "CGALRenderer.h"
#include "cgal.h"
#include "cgalutils.h"
#include "CGAL_Nef_polyhedron.h"

static void setupCamera(Camera &cam, const BoundingBox &bbox)
{
	PRINTDB("setupCamera() %i", static_cast<int>(cam.type));
	if (cam.type == Camera::CameraType::NONE) cam.viewall = true;
	if (cam.viewall) cam.viewAll(bbox);
}

bool export_png(const shared_ptr<const Geometry> &root_geom, Camera &cam, std::ostream &output)
{
	PRINTD("export_png geom");
	OffscreenView *glview;
	try {
		glview = new OffscreenView(cam.pixel_width, cam.pixel_height);
	} catch (int error) {
		fprintf(stderr,"Can't create OpenGL OffscreenView. Code: %i.\n", error);
		return false;
	}
	CGALRenderer cgalRenderer(root_geom);

	BoundingBox bbox = cgalRenderer.getBoundingBox();
	setupCamera(cam, bbox);

	glview->setCamera(cam);
	glview->setRenderer(&cgalRenderer);
	glview->setColorScheme(RenderSettings::inst()->colorscheme);
	glview->paintGL();
	glview->save(output);
	return true;
}

enum class Previewer { OPENCSG, THROWNTOGETHER } previewer;

#ifdef ENABLE_OPENCSG
#include "OpenCSGRenderer.h"
#include <opencsg.h>
#endif
#include "ThrownTogetherRenderer.h"

bool export_png_preview_common(Tree &tree, Camera &cam, std::ostream &output, Previewer previewer = Previewer::OPENCSG)
{
	OpenCSG::setOption(OpenCSG::AlgorithmSetting, OpenCSG::Goldfeather);

	PRINTD("export_png_preview_common");
	CsgInfo csgInfo = CsgInfo();
	csgInfo.compile_products(tree);

	OffscreenView *glview;
	try {
		glview = new OffscreenView(cam.pixel_width, cam.pixel_height);
	} catch (int error) {
		fprintf(stderr,"Can't create OpenGL OffscreenView. Code: %i.\n", error);
		return false;
	}

#ifdef ENABLE_OPENCSG
	OpenCSGRenderer openCSGRenderer(csgInfo.root_products, csgInfo.highlights_products, csgInfo.background_products, glview->shaderinfo);
#endif
	ThrownTogetherRenderer thrownTogetherRenderer(csgInfo.root_products, csgInfo.highlights_products, csgInfo.background_products);

#ifdef ENABLE_OPENCSG
	if (previewer == Previewer::OPENCSG)
		glview->setRenderer(&openCSGRenderer);
	else
#endif
		glview->setRenderer(&thrownTogetherRenderer);
#ifdef ENABLE_OPENCSG
	BoundingBox bbox = glview->getRenderer()->getBoundingBox();
	setupCamera(cam, bbox);

	if (cam.type == Camera::SIMPLE) {
  	double radius = 1.0;
  	if (csgInfo.root_chain) {
  		// BoundingBox bbox = csgInfo.root_chain->getBoundingBox();
  		cam.center = (bbox.min() + bbox.max()) / 2;
  		radius = (bbox.max() - bbox.min()).norm() / 2;
  	}

	  float angle1 = (cam.rotx * M_PI)/180.0;

    // fudge z angle and zoom in a bit on extreme angles, TODO: There is a better way to do this  :)
    float zangle1 = (-cam.rotz*2 * M_PI)/180.0;
    if (cam.rotz >= 90.0 || cam.rotz <= -90.0) {
			// TOP down
      radius = radius/3.0;
    }
    if (cam.rotz >= 180.0 || cam.rotz <= -180.0) {
      radius = radius/4.0;
    }

    // Vector3d cameradir(cos(angle1)-sin(angle1), sin(angle1)+cos(angle1), -0.95);
    Vector3d cameradir(cos(angle1)-sin(angle1), sin(angle1)+cos(angle1), zangle1);
    // cam.eye = cam.center - radius * 1.8 * cameradir;
    cam.eye = cam.center - radius * 3.2 * cameradir;

		cam.type = Camera::VECTOR;
  }

	csgInfo.glview->setCamera(cam);
	OpenCSG::setContext(0);
	OpenCSG::setOption(OpenCSG::OffscreenSetting, OpenCSG::FrameBufferObject);
#endif
	glview->setColorScheme(RenderSettings::inst()->colorscheme);
	glview->paintGL();
	glview->save(output);
	return true;
}

bool export_png_with_opencsg(Tree &tree, Camera &cam, std::ostream &output)
{
	PRINTD("export_png_w_opencsg");
#ifdef ENABLE_OPENCSG
	return export_png_preview_common(tree, cam, output, Previewer::OPENCSG);
#else
	fprintf(stderr,"This openscad was built without OpenCSG support\n");
	return false;
#endif
}

bool export_png_with_throwntogether(Tree &tree, Camera &cam, std::ostream &output)
{
	PRINTD("export_png_w_thrown");
	return export_png_preview_common(tree, cam, output, Previewer::THROWNTOGETHER);
}

#endif // ENABLE_CGAL

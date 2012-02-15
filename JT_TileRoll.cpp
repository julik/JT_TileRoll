// JT_TileRolls the input by an integer number of pixels.

#include "DDImage/Iop.h"
#include "DDImage/Row.h"
#include "DDImage/Pixel.h"
#include "DDImage/Knobs.h"
#include "DDImage/Knob.h"
#include "DDImage/Vector2.h"
#include "DDImage/DDMath.h"
#include "DDImage/Hash.h"

#include <vector>
#include <algorithm>

using namespace DD::Image;

static const char* const CLASS = "JT_TileRoll";
static const char* const HELP = "Rolls the input by an integer number of pixels and loops it, for painting away tiling";

class JT_TileRoll : public Iop
{
	void _validate(bool);
	virtual void _request(int, int, int, int, ChannelMask, int);
	virtual void engine(int y, int x, int r, ChannelMask, Row & t);
	double x, y;
	double x0, y0;
	int dx, dy;
	Matrix4 matrix_;
	
	
public:
	
	JT_TileRoll(Node* node) : Iop(node) { 
		x = y = x0 = y0 = 0;
	}
	
	void append(Hash& hash); // Hashing overloaded
	signed int offsetCoord(signed int coord, int total);
	virtual void knobs(Knob_Callback);
	const char* Class() const { return CLASS; }
	const char* node_help() const { return HELP; }
	static const Iop::Description d;
	Matrix4* matrix() { return &matrix_; }
	int slowness() const { return 1; } // this is a somewhat fast operator...
};

// Hashing for caches. We append our version to the cache hash, so that when you update
// the plugin all the caches will(should?) be flushed automatically
void JT_TileRoll::append(Hash& hash) {
	hash.append(__DATE__);
	hash.append(__TIME__);
	Iop::append(hash); // the super called he wants his pointers back
}

void JT_TileRoll::_validate(bool)
{
	// Figure out the integer translations. Floor(x+.5) is used so that
	// values always round the same way even if negative and even if .5
	dx = int(floor(x - x0 + .5));
	dy = int(floor(y - y0 + .5));
	
	copy_info();
	
	// create the transformation matrix for the GUI:
	matrix_.translation(x, y);
	
	// enforce the same bbox
	Box b(input0().info().x(), input0().info().y(), input0().info().r(), input0().info().t());
	info_.intersect(b);
}

signed int JT_TileRoll::offsetCoord(signed int coord, int total)
{
	if (coord < 0) {
		coord = coord % total;
		coord = total + coord;
	}
	if(coord > total) {
		coord = coord % total;
	}
	return coord;
}

void JT_TileRoll::_request(int x, int y, int r, int t, ChannelMask channels, int count)
{
	// get the whole thing
	input0().request(channels, count);
}

void JT_TileRoll::engine ( int y, int x, int r, ChannelMask channels, Row& out )
{
	Pixel pixel(channels);
	signed int theX, theY;

	theY = y - dy;
	theY = offsetCoord(theY, input0().format().height());
	
	for (; x < r; x++) {
		theX = x - dx;
		theX = offsetCoord(theX, input0().format().width());
		
		input0().at(theX, theY, pixel);
		// write the resulting pixel into the image
		foreach (z, channels) {
			float* destBuf = out.writable(z);
			*(destBuf+x)  = pixel[z];
		}
	}
}

void JT_TileRoll::knobs(Knob_Callback f)
{
	XY_knob(f, &x, "roll");
	XY_knob(f, &x0, 0, INVISIBLE);
	Tooltip(f, "The number of pixels by which the canvas has to be offset. This is rounded to the nearest number of pixels, no filtering is done.");
}

static Iop* build(Node* node) { return new JT_TileRoll(node); }
const Iop::Description JT_TileRoll::d(CLASS, "Transform/JT_TileRoll", build);

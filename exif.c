#include <u.h>
#include <libc.h>
#include <bio.h>

typedef struct IfdRec	IfdRec;
typedef struct ExifData	ExifData;
typedef struct EnumVal EnumVal;

enum{
	BigEndian,
	LittleEndian,
};

enum{
 	ExifTag=0x8769,
	GpsTag=0x8825,
};

enum{
 	ImageDescription=0x010e,
 	Make=0x010f,
 	Model=0x0110,
 	Orientation=0x0112,
 	XResolution=0x011a,
 	YResolution=0x011b,
	ResolutionUnit=0x0128,
 	Software=0x0131,
 	DateTime=0x0132,
	Artist=0x013b,
 	WhitePoint=0x013e,
 	PrimaryChromaticities=0x013f,
 	YCbCrCoefficients=0x0211,
 	YCbCrPositioning=0x0213,
 	ReferenceBlackWhite=0x0214,
 	Copyright=0x8298,
	ExposureTime=0x829a,
	FNumber=0x829d,
	ExposureProgram=0x8822,
	ISOSpeedRatings=0x8827,
	SensitivityType=0x8830,
	DateTimeOriginal=0x9003,
	DateTimeDigitized=0x9004,
	CompressedBitsPerPixel=0x9102,
	ExposureBiasValue=0x9204,
	MaxApertureValue=0x9205,
	MeteringMode=0x9207,
	LightSource=0x9208,
	Flash=0x9209,
	FocalLength=0x920a,
	MakerNote=0x927c,
	ColorSpace=0xa001,
	PixelXDimension=0xa002,
	PixelYDimension=0xa003,
	InteroperabilityTag=0xa005,
	SensingMethod=0xa217,
	CustomRendered=0xa401,
	ExposureMode,
	WhiteBalance,
	DigitalZoomRatio,
	FocalLengthIn35mmFilm,
	SceneCaptureType,
	GainControl,
	Contrast,
	Saturation,
	Sharpness,
	Altitude=0x0006,
	LatitudeHemisphere=0x0001,
	Latitude=0x0002,
	LongitudeHemisphere=0x0003,
	Longitude=0x0004,
	ImageDirection=0x0011,
	HorizontalPositioningError=0x001f,
};

enum{
	Tbyte = 1,
	Tascii = 2,
	Tshort = 3,
	Tlong = 4,
	Tratio = 5,
	Tundef = 7,
	Tslong = 9,
	Tsratio = 10,
};

static int prenum(IfdRec*, int, int, void*);
static int prascii(IfdRec*, int, int, void*);
static int prbyte(IfdRec*, int, Biobuf*, void*);
static int prshort(IfdRec*, int, int, void*);
static int prlong(IfdRec*, int, int, void*);
static int prratio(IfdRec*, int, int, void*);
static int prslong(IfdRec*, int, int, void*);
static int prsratio(IfdRec*, int, int, void*);
static int prdec(IfdRec*, int, int, void*);
static int prundef(IfdRec*, int, int, void*);
static int prifdrec(IfdRec*, int, Biobuf*);

struct IfdRec
{
	u16int tag;
	u16int fmt;
	u32int cnt;
	u8int val[4];
	u32int ival;
};

struct ExifData
{
	u16int tag;
	char str[32];
	uchar show;
	int (*print)(IfdRec*, int, int, void*);
	void *aux;
};

struct EnumVal
{
	u32int val;
	char *str;
};

int aflag, uflag, rflag;

static EnumVal orientations[] = {
	{ 1, "upper left" },
	{ 3, "lower right" },
	{ 6, "upper right" },
	{ 8, "lower left" },
	{ 9, "undefined" },
	{ 0, nil }
};

static EnumVal resunits[] = {
	{ 1, "no unit" },
	{ 2, "inches" },
	{ 3, "centimeters" },
	{ 0, nil }
};

static EnumVal ycbcrpos[] = {
	{ 1, "centered" },
	{ 2, "co-sited" },
	{ 0, nil }
};

static ExifData exiftab[] = {
	{ Artist, "Artist", 0 },
	{ DateTime, "DateTime", 1 },
	{ ImageDescription, "ImageDescription", 1 },
	{ Make, "Make", 1 },
	{ Model, "Model", 1 },
	{ Orientation, "Orientation", 0, prenum, orientations },
	{ ResolutionUnit, "ResolutionUnit", 0, prenum, resunits },
	{ Software, "Software", 1 },
	{ XResolution, "XResolution", 0, prdec },
	{ YResolution, "YResolution", 0, prdec },
	{ YCbCrPositioning, "YCbCrPositioning", 0, prenum, ycbcrpos },
	{ ExposureTime, "ExposureTime", 0, prdec, "s" },
	{ FNumber, "FNumber", 0, prdec },
	{ ExposureProgram, "ExposureProgram" },
	{ ISOSpeedRatings, "ISOSpeedRatings" },
	{ SensitivityType, "SensitivityType" },
	{ DateTimeOriginal, "DateTimeOriginal" },
	{ DateTimeDigitized, "DateTimeDigitized" },
	{ CompressedBitsPerPixel, "CompressedBitsPerPixel", 0, prdec },
	{ ExposureBiasValue, "ExposureBiasValue", 0, prdec },
	{ MaxApertureValue, "MaxApertureValue", 0, prdec },
	{ MeteringMode, "MeteringMode" },
	{ LightSource, "LightSource" },
	{ Flash, "Flash" },
	{ FocalLength, "FocalLength", 0, prdec, "mm" },
	{ ColorSpace, "ColorSpace" },
	{ PixelXDimension, "PixelXDimension" },
	{ PixelYDimension, "PixelYDimension" },
	{ InteroperabilityTag, "InteroperabilityTag" },
	{ SensingMethod, "SensingMethod" },
	{ CustomRendered, "CustomRendered" },
	{ ExposureMode, "ExposureMode" },
	{ WhiteBalance, "WhiteBalance" },
	{ DigitalZoomRatio, "DigitalZoomRatio", 0, prdec },
	{ FocalLengthIn35mmFilm, "FocalLengthIn35mmFilm" },
	{ SceneCaptureType, "SceneCaptureType" },
	{ GainControl, "GainControl" },
	{ Contrast, "Contrast" },
	{ Saturation, "Saturation" },
	{ Sharpness, "Sharpness" },
	{ Altitude, "Altitude", 1, prdec, "m" },
	{ LatitudeHemisphere, "LatitudeHemisphere", 1},
	{ Latitude, "Latitude", 1, prdec},
	{ LongitudeHemisphere, "LongitudeHemisphere", 1},
	{ Longitude, "Longitude", 1, prdec},
	{ ImageDirection, "ImageDirection", 1, prdec},
	{ HorizontalPositioningError, "HorizontalPositioningError", 0, prdec},

	{ 0 }
};

static u16int
get16(uchar *b, int e)
{
	if(e == BigEndian)
		return (u16int)b[0]<<8 | b[1];
	return (u16int)b[1]<<8 | b[0];
}

static int
read16(Biobuf *r, int e, u16int *v)
{
	u8int b[2];

	if(Bread(r, b, 2) != 2)
		return -1;
	if(v != nil)
		*v = get16(b, e);
	return 2;
}

static u32int
get32(uchar *b, int bo)
{
	u32int h1, h2;

	h1 = get16(&b[0], bo);
	h2 = get16(&b[2], bo);
	if(bo == BigEndian)
		return h1<<16 | h2;
	return h2<<16 | h1;
}

static int
read32(Biobuf *r, int e, u32int *v)
{
	u8int b[4];

	if(Bread(r, b, 4) != 4)
		return -1;
	if(v != nil)
		*v = get32(b, e);
	return 4;
}

static int
prenum(IfdRec *r, int, int, void *aux)
{
	EnumVal *e;

	e = aux;
	if(r->cnt == 1){
		while(e->str != nil){
			if(e->val == r->ival)
				return print("%s", e->str);
			e++;
		}
	}
	return print("unknown");
}

static char*
fmtstr(u8int fmt)
{
	switch(fmt){
	case Tbyte: return "byte";
	case Tascii: return "ascii";
	case Tshort: return "short";
	case Tlong: return "long";
	case Tratio: return "ratio";
	case Tundef: return "undef";
	case Tslong: return "slong";
	case Tsratio: return "sratio";
	}
	return "inval";
}

static int
prifdrec(IfdRec *r, int bo, Biobuf *b)
{
	ExifData *e;

	for(e = exiftab; e->tag != 0; e++){
		if(e->tag == r->tag)
			break;
	}
	if(e->show || aflag){
		if(*e->str != 0){
			print("%s: ", e->str);
		}else if(uflag)
			print("unknown(0x%04uhx,%s): ", r->tag, fmtstr(r->fmt));
		else
			return 0;

		if(rflag == 0 && e->print)
			e->print(r, bo, b->fid, e->aux);
		else
			switch(r->fmt){
			case Tbyte:
				prbyte(r, bo, b, nil);
				break;
			case Tascii:
				prascii(r, bo, b->fid, nil);
				break;
			case Tshort:
				prshort(r, bo, b->fid, nil);
				break;
			case Tlong:
				prlong(r, bo, b->fid, nil);
				break;
			case Tratio:
				prratio(r, bo, b->fid, nil);
				break;
			case Tslong:
				prslong(r, bo, b->fid, nil);
				break;
			case Tsratio:
				prsratio(r, bo, b->fid, nil);
				break;
			case Tundef:
				prundef(r, bo, b->fid, nil);
				break;
			}
		print("\n");
	}
	return 0;
}

static int
prlong0(IfdRec *r, int bo, int fd, void*, int s)
{
	int i;
	uchar buf[4];
	char *fmt;

	fmt = s ? "%d" : "%ud";
	switch(r->cnt){
	case 0:
		return 0;
	case 1:
		return print(fmt, get32(r->val, bo));
	}
	for(i = 0; i < r->cnt; i++){
		if(pread(fd, buf, 2, r->ival + 4*i) < 0)
			return -1;
		print(fmt, get32(buf, bo));
	}
	return 0;
}

static int
prslong(IfdRec *r, int bo, int fd, void *aux)
{
	return prlong0(r, bo, fd, aux, 1);
}

static int
prlong(IfdRec *r, int bo, int fd, void *aux)
{
	return prlong0(r, bo, fd, aux, 0);
}

static int
prshort(IfdRec *r, int bo, int fd, void*)
{
	int i;
	uchar buf[2];

	switch(r->cnt){
	case 0:
		return 0;
	case 1:
		return print("%uhd", get16(r->val, bo));
	case 2:
		return print("%uhd %uhd", get16(r->val, bo), get16(r->val+2, bo));
	}
	for(i = 0; i < r->cnt; i++){
		if(pread(fd, buf, 2, r->ival + 2*i) < 0)
			return -1;
		print("%uhd", get16(buf, bo));
	}
	return 0;
}

static int
prratio0(IfdRec *r, int bo, int fd, void*, int s)
{
	int i;
	long n;
	uchar buf[256];

	n = sizeof(buf);
	if(8*r->cnt < n)
		n = 8*r->cnt;
	if((n = pread(fd, buf, n, r->ival)) < 0)
		return -1;
	for(i = 0; i < n; i += 8){
		if(i > 0)
			print(" ");
		print(s ? "%d/%d" : "%ud/%ud", get32(&buf[i], bo), get32(&buf[i+4], bo));
	}
	return 0;
}

static int
prsratio(IfdRec *r, int bo, int fd, void *aux)
{
	return prratio0(r, bo, fd, aux, 1);
}

static int
prratio(IfdRec *r, int bo, int fd, void *aux)
{
	return prratio0(r, bo, fd, aux, 0);
}

static int
prdec0(IfdRec *r, int bo, int fd, void *aux, int s)
{
	int i;
	char *suffix;
	uchar buf[8];
	u32int n, d;
	float f;

	suffix = aux;
	if(suffix == nil)
		suffix = "";
	for(i = 0; i < r->cnt; i++){
		if(pread(fd, buf, 8, r->ival + 8*i) < 0)
			return -1;
		if(i > 0)
			print(" ");
		n = get32(&buf[i], bo);
		d = get32(&buf[i+4], bo);
		f = s ? (float)(long)n/(long)d : (float)n/d;
		print("%g%s", f, suffix);
	}
	return 0;
}

static int
prdec(IfdRec *r, int bo, int fd, void *aux)
{
	return prdec0(r, bo, fd, aux, r->fmt == Tsratio);
}

static int
prbyte(IfdRec *r, int, Biobuf *b, void*)
{
	int c, i;

	if(r->cnt <= 4)
		return print("%c %c %c %c",
			r->val[0], r->val[1], r->val[2], r->val[3]);
	for(i = 0; i < r->cnt; i++){
		if((c = Bgetc(b)) < 0)
			return -1;
		if(i > 0)
			print(" ");
		print("%c", (char)c);
	}
	return 0;
}

static int
prascii(IfdRec *r, int, int fd, void*)
{
	long n;
	char buf[256];

	if(r->cnt <= 4){
		if(r->cnt > 1)
			return write(1, r->val, r->cnt - 1);
		return 0;
	}
	n = sizeof(buf);
	if(r->cnt < n)
		n = r->cnt;
	if((n = pread(fd, buf, n, r->ival)) < 0)
		return -1;
	return write(1, buf, n);
}

static int
prundef(IfdRec *r, int, int, void*)
{
	return print("cnt=%uhd val=(0x%uhhx 0x%uhhx 0x%uhhx 0x%uhhx)",
		r->cnt, r->val[0], r->val[1], r->val[2], r->val[3]);
}

static void
usage(void)
{
	fprint(2, "usage: %s file...\n", argv0);
	exits("usage");
}

static int
vparse(Biobuf *r, int e, char *fmt, va_list a)
{
	int c, i, n;
	u8int *x;
	u16int w;
	u32int dw;

	for(;;){
		n = 0;
		switch(*fmt++){
		case '\0':
			return 0;
		case 'b':
			if((c = Bgetc(r)) < 0)
				return -1;
			*va_arg(a, uchar*) = (uchar)c;
			break;
		case 'w':
			if(read16(r, e, &w) < 0)
				return -1;
			*va_arg(a, u16int*) = w;
			break;
		case 'W':
			if(read32(r, e, &dw) < 0)
				return -1;
			*va_arg(a, u32int*) = dw;
			break;
		case 'X':
			n += 2;
		case 'x':
			n += 2;
			x = va_arg(a, uchar*);
			for(i = 0; i < n; i++){
				if((c = Bgetc(r)) < 0)
					return -1;
				*x++ = (uchar)c;
			}
			break;
		}
	}
}

static int
parse(Biobuf *r, int e, char *fmt, ...)
{
	int n;
	va_list a;

	va_start(a, fmt);
	n = vparse(r, e, fmt, a);
	va_end(a);
	return n;
}

static int
dumpinfo(Biobuf *r)
{
	char buf[64];
	u16int w, soi, appn, len, ver, nents;
	u32int segoff, ifdoff, exiftag, gpstag;
	int i, bo, total;
	IfdRec rec;

	bo = BigEndian;
	total = 0;
	if(parse(r, bo, "www", &soi, &appn, &len) < 0)
		goto Badfmt;
	if(soi != 0xffd8)
		goto Badfmt;
	for(;;){
		if((appn&0xfff0) != 0xffe0)
			goto Badfmt;
		if((appn&0xf) == 1)
			break;
		if(len < 2)
			goto Badfmt;
		len -= 2;
		while(len-- > 0){
			if(Bgetc(r) < 0)
				goto Badfmt;
		}
		if(parse(r, bo, "ww", &appn, &len) < 0)
			goto Badfmt;
	}
	if(Bread(r, buf, 6) != 6 || memcmp(buf, "Exif\0\0", 6) != 0)
		goto Badfmt;
	segoff = Boffset(r);
	if(read16(r, bo, &w) < 0)
		goto Badfmt;
	switch(w){
	case 0x4949:
		bo = LittleEndian;
		break;
	case 0x4d4d:
		bo = BigEndian;
		break;
	default:
		goto Badfmt;
	}
	if(parse(r, bo, "wW", &ver, &ifdoff) < 0)
		goto Badfmt;
	if(ver != 42 || ifdoff != 8)
		goto Badfmt;
	exiftag = 0;
	gpstag = 0;
	for(;;){
		if(parse(r, bo, "w", &nents) < 0)
			goto Badfmt;
		for(i = 0; i < nents; i++){
			memset(&rec, 0, sizeof(rec));
			if(parse(r, bo, "wwWX", &rec.tag, &rec.fmt, &rec.cnt, rec.val) < 0)
				goto Badfmt;
			rec.ival = segoff + get32(rec.val, bo);
			switch(rec.tag){
			case ExifTag:
				exiftag = rec.ival;
				break;
			case GpsTag:
				gpstag = rec.ival;
				break;
			default:
				prifdrec(&rec, bo, r);
				break;
			}
			total++;
		}
		if(parse(r, bo, "w", &ifdoff) < 0)
			goto Badfmt;
		if(ifdoff == 0){
			if(exiftag != 0){
				ifdoff = exiftag;
				exiftag = 0;
			}else if(gpstag != 0){
				ifdoff = gpstag;
				gpstag = 0;
			}else
				break;
		}
		Bseek(r, ifdoff, 0);
	}
	return total;
Badfmt:
	werrstr("unknown file format");
	return -1;
}

void
main(int argc, char **argv)
{
	int i;
	Biobuf *r;

	ARGBEGIN{
	case 'a':
		aflag++;
		break;
	case 'u':
		uflag++;
		break;
	case 'r':
		rflag++;
		break;
	default:
		usage();
	}ARGEND;
	if(argc < 1)
		usage();
	for(i = 0; i < argc; i++){
		r = Bopen(argv[i], OREAD);
		if(r == nil)
			sysfatal("open: %r");
		if(i > 0)
			print("\n");
		print("File: %s\n", argv[i]);
		switch(dumpinfo(r)){
		case -1:
			sysfatal("%r");
		case 0:
			fprint(2, "no exif data\n");
		}
		Bterm(r);
	}
}

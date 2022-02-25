#include "potracelib.h"
#include "main_potrace.h"
#include "auxiliary.h"


#ifdef __cplusplus
extern "C" {
#endif


struct line {
    point_t points[2];
};

struct curve {
    point_t points[4];
};

struct shape {

    char isLine; // 1 if line, 0 if curve

    //anonymous union
    union {
		point_t line[2];
		point_t curve[4];
    };
};

struct shapeList {
    struct shape *shapes;
    int size,currentIndex;
};


struct shapeList pageLatex(potrace_path_t* plist, imginfo_t* imginfo);

#ifdef  __cplusplus
} /* end of extern "C" */
#endif




#ifndef __cfg_h__
#define __cfg_h__


#include "svp.h"

extern gsf_svp_parm_t svp_parm;
extern char svp_parm_path[128];

// gsf_svp_parm_t
typedef struct {
    struct {
        int yolo_alg;
        int face_alg;  // control cho RetinaFace
        struct {
            float det_threshold;
            float nms_threshold;
            int max_faces;
            gsf_polygon_t det_polygon; // Vùng detect, tương tự YOLO
        } face_cfg;    // Cấu hình cho RetinaFace
    } svp;
    gsf_svp_yolo_t yolo;   // Cấu hình YOLO hiện tại
    gsf_svp_rface_t rface; //  cấu hình RetinaFace
} gsf_svp_parm_t;

int json_parm_load(char *filename, gsf_svp_parm_t *cfg);
int json_parm_save(char *filename, gsf_svp_parm_t *cfg);

#endif
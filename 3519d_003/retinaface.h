#ifndef __retinaface_h__
#define __retinaface_h__

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

#ifndef FACE_BOX_MAX
#define FACE_BOX_MAX 64
typedef struct {
  int chn;
  int w, h; 
  int size;
  struct {
    float score;
    float x;
    float y;
    float w;
    float h;
    float landmarks[10]; // 5 điểm landmark của face
  } box[FACE_BOX_MAX];
} face_boxs_t;
#endif

#define FACE_CHN_MAX 9
int retinaface_init(int vpss_grp[FACE_CHN_MAX], int vpss_chn[FACE_CHN_MAX], char *ModelPath);
int retinaface_detect(face_boxs_t boxs[FACE_CHN_MAX]); 
int retinaface_deinit();

#ifdef __cplusplus
}
#endif  /* __cplusplus */



#endif // __retinaface_h__

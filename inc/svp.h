#ifndef __svp_h__
#define __svp_h__

#ifdef __cplusplus
extern "C" {
#endif

#include "inc/gsf.h"

//for json cfg;
#include "mod/svp/inc/sjb_svp.h"


#define GSF_IPC_SVP      "ipc:///tmp/svp_rep"
#define GSF_PUB_SVP      "ipc:///tmp/svp_pub"

enum {
    GSF_ID_SVP_CFG    = 1,  // gsf_svp_t;
    GSF_ID_SVP_MD     = 2,  // ch, gsf_svp_md_t
    GSF_ID_SVP_LPR    = 3,  // ch, gsf_svp_lpr_t
    GSF_ID_SVP_YOLO   = 4,  // ch, gsf_svp_yolo_t
    GSF_ID_SVP_CFACE  = 5,  // ch, gsf_svp_cface_t
    GSF_ID_SVP_FACE   = 6,  // ch, /path/xxx.jpg;
    GSF_ID_SVP_FEATURE= 7,  //
    GSF_ID_SVP_RFACE  = 8,
    GSF_ID_SVP_END
};

enum {
    GSF_SVP_ERR = -1,
};


////////////////////////

enum {
  GSF_EV_SVP_MD  = 1, // gsf_svp_mds_t;
  GSF_EV_SVP_LPR = 2, // gsf_svp_lprs_t;
  GSF_EV_SVP_YOLO= 3, // gsf_svp_yolos_t;
  GSF_EV_SVP_CFACE=4, // gsf_svp_cfaces_t;
  GSF_EV_SVP_RFACE = 5, // gsf_svp_faces_t;
};

typedef struct {
  int thr;        //thr 
  int rect[4];    //����λ��
}gsf_md_result_t;

typedef struct {
  int pts;   // u64PTS/1000
  int cnt;
  int w, h;
  gsf_md_result_t result[20];
}gsf_svp_mds_t;

typedef struct {
	char 	  number[16];         //���ƺ���
	char 	  color[8];  		      //������ɫ
	float   number_realty;      //�������Ŷ�
	int     rect[4];            //��������
	int     type ;              //��������
	float   letter_realty[16];  //�ַ����Ŷ�
	float   vertangle;          //��ֱ�Ƕ�
	float   horzangle;          //ˮƽ�Ƕ�
}gsf_lpr_result_t;

typedef struct {
  int pts;   // u64PTS/1000
  int cnt;
  int w, h;
  gsf_lpr_result_t result[20];
}gsf_svp_lprs_t;

typedef struct {
  float score;
  int rect[4];
  char label[32];
}gsf_yolo_box_t;

typedef struct {
  int pts;   // u64PTS/1000
  int cnt;
  int w, h;
  gsf_yolo_box_t box[64];
}gsf_svp_yolos_t;


typedef struct {
  int id;
  int confidence;
  int rect[4];
}gsf_cface_box_t;

typedef struct {
  int pts;   // u64PTS/1000
  int cnt;
  int w, h;
  gsf_cface_box_t box[64];
}gsf_svp_cfaces_t;

// struct cho RetinaFace boxes
typedef struct {
    float score;
    int rect[4];          // x, y, w, h
    float landmarks[10];  // 5 điểm landmark (x,y)
} gsf_rface_box_t;
// struct cho RetinaFace results
typedef struct {
    int pts;   // u64PTS/1000
    int cnt;
    int w, h;
    gsf_rface_box_t box[64];
} gsf_svp_rfaces_t;

////////////////////////

#ifdef __cplusplus
}
#endif

#endif
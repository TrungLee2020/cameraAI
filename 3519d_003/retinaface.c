#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/timeb.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include "retinaface.h"
#include "svp.h"
#include "cfg.h"

extern void* svp_pub;

static pthread_t s_hMdThread = 0;
static int s_bStopSignal = 0;
static void* pHand = NULL;
static void* face_task(void* p);
static FILE* face_log_fp = NULL;


// Macro log riêng cho RetinaFace
#define FACE_LOGD(fmt, ...) \
    do { \
        if(face_log_fp) { \
            fprintf(face_log_fp, "[FACE][D][%s:%d] " fmt "\n", \
                    __func__, __LINE__, ##__VA_ARGS__); \
            fflush(face_log_fp); \
        } \
    } while(0)

#define FACE_LOGE(fmt, ...) \
    do { \
        if(face_log_fp) { \
            fprintf(face_log_fp, "[FACE][E][%s:%d] " fmt "\n", \
                    __func__, __LINE__, ##__VA_ARGS__); \
            fflush(face_log_fp); \
        } \
    } while(0)


// Channel config
#if defined(GSF_DEV_NVR) 
#warning "GSF_DEV_NVR"
int VpssGrp[FACE_CHN_MAX] = {0, 1, 2, 3, 4, 5, 6, 7, 8};
int VpssChn[FACE_CHN_MAX] = {0, 0, 0, 0, 0, 0, 0, 0, 0};
#else
#warning "GSF_DEV_IPC"
int VpssGrp[FACE_CHN_MAX] = {0, -1, -1, -1, -1, -1, -1, -1, -1};
int VpssChn[FACE_CHN_MAX] = {1, -1, -1, -1, -1, -1, -1, -1, -1};
#endif


int face_start(char *home_path)
{
  // Mở file log riêng
  char log_path[256];
  sprintf(log_path, "%s/log/retinaface.log", home_path);
  face_log_fp = fopen(log_path, "a+");
  
  s_bStopSignal = 0;
  return pthread_create(&s_hMdThread, NULL, face_task, (void*)home_path);
}


int face_load(char *home_path)
{
  char ModelPath[256];
  sprintf(ModelPath, "%s/model/retinaface.om", home_path);
  
  FACE_LOGD("ModelPath:[%s]", ModelPath);
  if(retinaface_init(VpssGrp, VpssChn, ModelPath) < 0)
  {
      FACE_LOGE("retinaface_init err.");
      return -1;
  }
  return 0;
}

static int pub_send(face_boxs_t *boxs)
{
  int i = 0, j = 0;
  char buf[sizeof(gsf_msg_t) + sizeof(gsf_svp_faces_t)]; 
  gsf_msg_t *msg = (gsf_msg_t*)buf;
  
  memset(msg, 0, sizeof(*msg));
  msg->id = GSF_EV_SVP_RFACE;
  msg->ts = time(NULL)*1000;
  msg->sid = 0;
  msg->err = 0;
  msg->size = sizeof(gsf_svp_faces_t);
  msg->ch = boxs->chn;
  
  gsf_svp_faces_t *faces = (gsf_svp_faces_t*)msg->data;
  
  faces->pts = 0;
  faces->w = boxs->w;
  faces->h = boxs->h;
  faces->cnt = boxs->size;
  faces->cnt = (faces->cnt > sizeof(faces->box)/sizeof(faces->box[0]))
               ?sizeof(faces->box)/sizeof(faces->box[0])
               :faces->cnt;

  for(i = 0; i < faces->cnt; i++)
  {
    // Kiểm tra nếu có polygon được cấu hình
    if(svp_parm.face_cfg.det_polygon.polygon_num && 
       svp_parm.face_cfg.det_polygon.polygons[0].point_num)
    {
        // Lấy số điểm của polygon
        int pn = svp_parm.face_cfg.det_polygon.polygons[0].point_num;
        
        // Khởi tạo mảng chứa tọa độ của polygon (P) và face box (Q)
        double P[10*2] = {0};  // Polygon có thể có tới 10 điểm
        double Q[4*2] = {0};   // Face box có 4 điểm
        
        // Chuyển đổi tọa độ polygon từ tỉ lệ sang pixel
        for(int p = 0; p < pn; p++)
        {
            P[p*2+0] = svp_parm.face_cfg.det_polygon.polygons[0].points[p].x * faces->w;
            P[p*2+1] = svp_parm.face_cfg.det_polygon.polygons[0].points[p].y * faces->h;
        }
        
        // Tạo 4 điểm của face box
        Q[0] = boxs->box[i].x;                           // Top-left x
        Q[1] = boxs->box[i].y;                           // Top-left y
        Q[2] = (boxs->box[i].x + boxs->box[i].w);       // Top-right x
        Q[3] = boxs->box[i].y;                           // Top-right y
        Q[4] = (boxs->box[i].x + boxs->box[i].w);       // Bottom-right x
        Q[5] = (boxs->box[i].y + boxs->box[i].h);       // Bottom-right y
        Q[6] = boxs->box[i].x;                          // Bottom-left x
        Q[7] = (boxs->box[i].y + boxs->box[i].h);       // Bottom-left y

        // Tính toán IoU giữa face box và polygon
        double iou = poly_iou(P, pn*2, Q, 4*2);
        
        // Bỏ qua face này nếu IoU quá nhỏ (không nằm trong vùng quan tâm)
        if(iou < 0.001)
            continue;
    }
    
    // Nếu face nằm trong vùng quan tâm hoặc không có polygon, copy dữ liệu
    faces->box[j].score = boxs->box[i].score;
    faces->box[j].rect[0] = boxs->box[i].x;
    faces->box[j].rect[1] = boxs->box[i].y;
    faces->box[j].rect[2] = boxs->box[i].w;
    faces->box[j].rect[3] = boxs->box[i].h;
    
    // Copy landmarks
    memcpy(faces->box[j].landmarks, boxs->box[i].landmarks, 
           sizeof(float)*10);
    
    j++;
  }
  
  // Cập nhật số lượng face thực tế sau khi lọc
  faces->cnt = j;

  nm_pub_send(svp_pub, (char*)msg, sizeof(*msg)+msg->size);
  return 0;
}

static void* face_task(void* p)
{
  if(face_load((char*)p) < 0)
    return NULL;
    
  while (0 == s_bStopSignal)
  {
      face_boxs_t boxs[FACE_CHN_MAX] = {0};
      
      int ret = retinaface_detect(boxs);

      for(int i = 0; i < ret; i++)
      {
        pub_send(&boxs[i]);
      }
  }
  
  // Clean up
  face_boxs_t boxs[FACE_CHN_MAX] = {0};
  for(int i = 0; i < FACE_CHN_MAX; i++)
  {
    if(VpssGrp[i] >= 0)
    {  
      boxs[i].chn = i;
      pub_send(&boxs[i]);
    }
  }
  
  retinaface_deinit();
  return NULL;
}

int face_stop()
{
  if(s_hMdThread)
  {
    s_bStopSignal = 1;
    pthread_join(s_hMdThread, NULL);
    s_hMdThread = 0;
  }

  if(face_log_fp) {
    fclose(face_log_fp);
    face_log_fp = NULL;
  }
  return 0;
}
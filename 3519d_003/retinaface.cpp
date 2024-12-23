#include "retinaface.h"
#include "sample_svp_npu_process.h"
#include "hi_comm_video.h"
#include "mpi_sys.h"
#include "vpss_capture.h"

typedef struct {
  int vpss_grp, vpss_chn;
  VpssCapture vcap;
  cv::Mat image;
  int fd;
} vcap_face_t;

static vcap_face_t vcap[FACE_CHN_MAX];

int retinaface_init(int vpss_grp[FACE_CHN_MAX], int vpss_chn[FACE_CHN_MAX], char *ModelPath)
{
    int i = 0;
    int ret = -1;
   
    // Khởi tạo VPSS channels
    for(i = 0; i < FACE_CHN_MAX; i++)
    {
      vcap[i].vpss_grp = vpss_grp[i];
      vcap[i].vpss_chn = vpss_chn[i];
    }

    // Khởi tạo NPU với model RetinaFace
    ret = sample_svp_npu_init(ModelPath);
    printf("sample_svp_npu_init ret:%d [%s]\n", ret, ModelPath);
    
    return ret;
}

int retinaface_detect(face_boxs_t _boxs[FACE_CHN_MAX])
{
    static struct timespec _ts1, _ts2;
    clock_gettime(CLOCK_MONOTONIC, &_ts2);
    int check_fd = (_ts2.tv_sec - _ts1.tv_sec > 3)?1:0;
    
    hi_video_frame_info *hi_frame = NULL, *other_frame = NULL;
    face_boxs_t *boxs = _boxs;
    int ret = 0;
    int maxfd = 0;
    
    // Setup fd_set for select()
    fd_set read_fds;
    FD_ZERO(&read_fds);
    
    // Check và update file descriptors
    for (int i = 0; i < FACE_CHN_MAX; i++)
    {
      if(vcap[i].vpss_grp == -1)
        continue;
      
      if(check_fd)
      {
        _ts1 = _ts2;
        int fd_new = hi_mpi_vpss_get_chn_fd(vcap[i].vpss_grp, vcap[i].vpss_chn);
        if(vcap[i].fd != fd_new)
        {
          printf("check vpss_grp:%d, vpss_chn:%d, fd:[%d => %d]\n", 
                 vcap[i].vpss_grp, vcap[i].vpss_chn, vcap[i].fd, fd_new);
          vcap[i].vcap.destroy();
          vcap[i].fd = vcap[i].vcap.init(vcap[i].vpss_grp, vcap[i].vpss_chn, 0, 0, 1);
        }
      }
      
      if(vcap[i].fd > 0)
      {  
        FD_SET(vcap[i].fd, &read_fds);
        maxfd = (maxfd < vcap[i].fd)?vcap[i].fd:maxfd;
      }
    }
    
    // Wait for frame with timeout
    struct timeval to;
    to.tv_sec = 2; 
    to.tv_usec = 0;
    
    ret = select(maxfd + 1, &read_fds, NULL, NULL, &to);
    if (ret < 0)
    {
        printf("vpss select failed!\n");
        return -1;
    }
    else if (ret == 0)
    {
        printf("vpss get timeout!\n");
        return 0;
    }
    
    // Process frames
    for (int i = 0; i < FACE_CHN_MAX; i++)
    {
      if(vcap[i].fd <= 0)
        continue;
      
      if (FD_ISSET(vcap[i].fd, &read_fds))
      {
        // Get frame
        vcap[i].vcap.get_frame_lock(NULL, &hi_frame, &other_frame);
        
        // Detect faces using NPU
        if(hi_frame)
        {
          // Thực hiện detection trên NPU
          sample_svp_npu_detect_faces(hi_frame, other_frame, boxs);
          boxs->chn = i;
          boxs++;
        }
        
        // Unlock frame
        vcap[i].vcap.get_frame_unlock(hi_frame, other_frame);
      }
    }
    
    return ((intptr_t)boxs-(intptr_t)_boxs)/sizeof(face_boxs_t);
}

int retinaface_deinit()
{
    // Destroy NPU resources
    int ret = sample_svp_npu_destroy();
    printf("sample_svp_npu_destroy ret:%d\n", ret);

    // Cleanup VPSS resources
    for(int i = 0; i < FACE_CHN_MAX; i++)
    {
        if(vcap[i].fd > 0)
        {  
            ret = vcap[i].vcap.destroy();
            printf("vcap[%d].destroy ret:%d\n", i, ret);
        }
    }
    
    return 0;
}
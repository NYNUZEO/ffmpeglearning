#include <stdio.h>
#include <SDL.h>

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>

#define REFRESH_EVENT  (SDL_USEREVENT + 1)

#define BREAK_EVENT  (SDL_USEREVENT + 2)

//clang -g -o a.out main.c `pkg-config --cflags --libs libavutil libavformat libavcodec libavutil libswscale libswresample sdl2`


int thread_exit = 0;
int thread_pause = 0;

int video_refresh_thread(void *data) {
    thread_exit = 0;
    thread_pause = 0;

    while (!thread_exit) {
        if (!thread_pause) {
            SDL_Event event;
            event.type = REFRESH_EVENT;
            SDL_PushEvent(&event);
        }
        SDL_Delay(33);
    }
    thread_exit = 0;
    thread_pause = 0;
    //Break
    SDL_Event event;
    event.type = BREAK_EVENT;
    SDL_PushEvent(&event);

    return 0;
}


int main(int argc, char *argv[]) {
    
    if(argc < 2)
    {
        printf("no path, done!\n");
        exit(1);
    }

    int ret = -1;

    AVFormatContext *pFormatCtx = NULL; 

    int i, videoStream;

    AVCodecParameters *pCodecParameters = NULL; 
    AVCodecContext *pCodecCtx = NULL;

    AVCodec *pCodec = NULL; 
    AVFrame *pFrame = NULL;
    AVPacket packet;

    SDL_Rect rect;
    Uint32 pixformat;

    char *file = argv[1];
    //渲染器
    SDL_Window *win = NULL;
    SDL_Renderer *renderer = NULL;
    SDL_Texture *texture = NULL;

    SDL_Thread *video_thread;
    SDL_Event event;

    
    int w_width = 233;
    int w_height = 233;

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Could not initialize SDL - %s\n", SDL_GetError());
        return ret;
    }

    if (avformat_open_input(&pFormatCtx, file, NULL, NULL) != 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to open video file!");
        goto __FAIL; 
    }

    videoStream = av_find_best_stream(pFormatCtx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);

    if (videoStream == -1) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Din't find a video stream!");
        goto __FAIL;
    }

    // 获取codecparameter
    pCodecParameters = pFormatCtx->streams[videoStream]->codecpar;

    // 获取codec
    pCodec = avcodec_find_decoder(pCodecParameters->codec_id);
    if (pCodec == NULL) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Unsupported codec!\n");
        goto __FAIL; // Codec not found
    }

    // 辅助context
    pCodecCtx = avcodec_alloc_context3(pCodec);
    if (avcodec_parameters_to_context(pCodecCtx, pCodecParameters) != 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't copy codec context");
        goto __FAIL;// Error copying codec context
    }

    // 打开解码器
    if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to open decoder!\n");
        goto __FAIL; // Could not open codec
    }

    pFrame = av_frame_alloc();

    w_width = pCodecCtx->width;
    w_height = pCodecCtx->height;

    win = SDL_CreateWindow("Play",
                           SDL_WINDOWPOS_UNDEFINED,
                           SDL_WINDOWPOS_UNDEFINED,
                           w_width, w_height,
                           SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    if (!win) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create window by SDL");
        goto __FAIL;
    }

    renderer = SDL_CreateRenderer(win, -1, 0);
    if (!renderer) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create Renderer by SDL");
        goto __FAIL;
    }

    pixformat = SDL_PIXELFORMAT_IYUV;
    texture = SDL_CreateTexture(renderer,
                                pixformat,
                                SDL_TEXTUREACCESS_STREAMING,
                                w_width,
                                w_height);

    SDL_CreateThread(video_refresh_thread, "Video Thread", NULL);


    while(1) {
        SDL_WaitEvent(&event);
        if (event.type == REFRESH_EVENT) {
            while (1) {
                if (av_read_frame(pFormatCtx, &packet) < 0)
                    thread_exit = 1;


                if (packet.stream_index == videoStream)
                    break;
            }

            if (packet.stream_index == videoStream) {

                avcodec_send_packet(pCodecCtx, &packet);
                while (avcodec_receive_frame(pCodecCtx, pFrame) == 0) {

                    SDL_UpdateYUVTexture(texture, NULL,
                                         pFrame->data[0], pFrame->linesize[0],
                                         pFrame->data[1], pFrame->linesize[1],
                                         pFrame->data[2], pFrame->linesize[2]);

                    // 设置
                    rect.x = 0;
                    rect.y = 0;
                    rect.w = pCodecCtx->width;
                    rect.h = pCodecCtx->height;

                    SDL_RenderClear(renderer);
                    SDL_RenderCopy(renderer, texture, NULL, &rect);
                    SDL_RenderPresent(renderer);
                }
                av_packet_unref(&packet);
            }
        } else if (event.type == SDL_KEYDOWN) {
            if (event.key.keysym.sym == SDLK_SPACE) {
                thread_pause = !thread_pause;
            }
            if (event.key.keysym.sym == SDLK_ESCAPE) {
                thread_exit = 1;
            }
        } else if (event.type == SDL_QUIT) {
            thread_exit = 1;
        } else if (event.type == BREAK_EVENT) {
            break;
        }
    }

__QUIT:
    return 0;

__FAIL:
    if (pFrame) 
        av_frame_free(&pFrame);
    if (pCodecCtx) 
        avcodec_close(pCodecCtx);
    if (pCodecParameters) 
        avcodec_parameters_free(&pCodecParameters);
    if (pFormatCtx) 
        avformat_close_input(&pFormatCtx);
    if (win) 
        SDL_DestroyWindow(win);
    if (renderer) 
        SDL_DestroyRenderer(renderer);
    if (texture)
        SDL_DestroyTexture(texture);
    SDL_Quit();
    return ret;
}
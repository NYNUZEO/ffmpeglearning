#include <stdio.h>
#include <unistd.h> 

#ifndef __RDT__
#define __RDT__
#include "/usr/local/include/SDL2/SDL.h"
#endif
#undef main
// SDL_Init()/SDL_Quit()
// SDL_CreateWindow()/SDL_DestoryWindow()
// SDL_CreateRender()
//
//          渲染器          交换
// 内存图像-》》》》》纹理-》》》》》》》窗口展示
// 

int main(int argc, char** argv)
{
    int quit = 1;
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window *window = NULL;
    SDL_Renderer *render = NULL;
    SDL_Event event;
    SDL_Texture *texture = NULL;
    SDL_Rect rect;

    //窗口名， 显示位置x， y， 窗口宽高x， y
    if(!(window = SDL_CreateWindow("SDL2 WINDOW", 200, 400, 640, 480, SDL_WINDOW_SHOWN)))
    {
        printf("bad create window!");
        goto err_quit;
    }
    
    if( !(render = SDL_CreateRenderer(window, -1, 0)) )
    {
        printf("bad create render");
        goto err_quit;
    }

   /* 
    SDL_SetRenderDrawColor(render, 255, 0, 0, 255);
    
    //刷/清空render(渲染器)
    SDL_RenderClear(render);

    //投影至屏幕
    SDL_RenderPresent(render);
    */

    texture = SDL_CreateTexture(render, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, 640, 480);
    int x = 0, y = 150, z = 200;
    do
    {
        //poll循环刷新队列头看是否有新的event
        SDL_PollEvent(&event);

        switch (event.type)
        {
        case SDL_QUIT:
            quit = 0;
            break;
        
        default:
            SDL_Log("event type is %d", event.type);
        }
        
        //设定一个矩形的参数特性
        rect.w = 30;
        rect.h = 30;
        rect.x = x % 640;
        rect.y = y % 480;

        //将纹理送入渲染器
        SDL_SetRenderTarget(render, texture);
        SDL_SetRenderDrawColor(render, (x / 10) % 256, y % 256, z % 256, 0);
        SDL_RenderClear(render);

        //用渲染器绘制方形
        SDL_RenderDrawRect(render, &rect);
        SDL_SetRenderDrawColor(render, 255, 0, 0, 0);
        SDL_RenderFillRect(render, &rect);

        //将渲染器重设为默认（窗口）
        SDL_SetRenderTarget(render, NULL);
        //将以渲染的纹理复制进渲染器
        SDL_RenderCopy(render, texture, NULL, NULL);

        //窗口显示
        SDL_RenderPresent(render);
        ++x, ++y;//, ++z;
        SDL_Delay(10);
    } while (quit);
   
    //sleep(6000);

err_quit:
    if(texture) 
        SDL_DestroyTexture(texture);

    if(window)
        SDL_DestroyWindow(window);

    SDL_Quit();
    return 0;
}
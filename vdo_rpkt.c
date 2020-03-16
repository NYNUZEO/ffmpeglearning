#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef __STDC_CONSTANT_MACROS
#define __STDC_CONSTANT_MACROS
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/avutil.h>
#endif

#define ERROR_STR_SIZE 1024
//STR string 存str文字的大小
#define ERROR_STR_SIZE 1024

char errinfo[1024] = {0};

#define DDug av_log(NULL, AV_LOG_WARNING, "Debug Now!\n");

int main(int argc, char **argv)
{
	int err_code = 0;
	int *stream_mapping = NULL;
	int stream_mapping_size = 0;
	char *src;
	char *dst;

	//注册所有组件 新版ffmpeg已经不需要提前注册了
	//av_register_all();

	//设置日志级别
	av_log_set_level(AV_LOG_INFO);

	//设置输出格式上下文
	AVOutputFormat *ofmt = NULL;
	//建立输入文件，输出文件的格式上下文
	AVFormatContext *ifmt_ctx = NULL, *ofmt_ctx = NULL;
	AVPacket pkt;

	//文件读取
	if (argc < 3)
	{
		av_log(NULL, AV_LOG_ERROR, "where your input / output path ?!\n");
		return -1;
	}

	src = argv[1];
	dst = argv[2];

	if (!src || !dst)
	{
		av_log(NULL, AV_LOG_ERROR, "file is null!\n");
		return -1;
	}

	//打开输入文件 并赋值给上下文ifmt_ctx
	if ((err_code = avformat_open_input(&ifmt_ctx, src, NULL, NULL)) < 0)
	{
		av_strerror(err_code, errinfo, 1024);
		av_log(NULL, AV_LOG_ERROR, "Can`t open file! ( %s )\n", errinfo);
		return -1;
	}

	//寻找流信息 确定有流
	if ((err_code = avformat_find_stream_info(ifmt_ctx, NULL)) < 0)
	{
		av_strerror(err_code, errinfo, 1024);
		av_log(NULL, AV_LOG_ERROR, "Can`t find stream in formation ( %s )\n", errinfo);
		return -1;
	}

	//输出输入文件信息
	av_dump_format(ifmt_ctx, 0, src, 0);

	//建立输出上下文，同时根据dst自动识别后缀构造文件类型
	avformat_alloc_output_context2(&ofmt_ctx, NULL, NULL, dst);

	if (!ofmt_ctx)
	{
		av_log(NULL, AV_LOG_ERROR, "Can`t create output context!\n");
		return -1;
	}

	//初始化映射数组和size 记录哪些流是需要的
	stream_mapping_size = ifmt_ctx->nb_streams;
	stream_mapping = (int *)(av_malloc_array(stream_mapping_size, sizeof(*stream_mapping)));
	int stream_index = 0;

	if (!stream_mapping)
	{
		av_log(NULL, AV_LOG_ERROR, "can`t molloc to streammapping\n");
		return -1;
	}

	//将ofmt设置为输出文件的容器格式
	ofmt = ofmt_ctx->oformat;

	for (int i = 0; i < ifmt_ctx->nb_streams; ++i)
	{
		AVStream *out_stream = NULL;
		AVStream *in_stream = ifmt_ctx->streams[i];			  //读取对应输入文件的流信息
		AVCodecParameters *in_codecpar = in_stream->codecpar; //读取当前流的输入编码参数

		//过滤非音频 字幕 视频的流信息
		int flag = (in_codecpar->codec_type == AVMEDIA_TYPE_AUDIO) + (in_codecpar->codec_type == AVMEDIA_TYPE_SUBTITLE) + (in_codecpar->codec_type == AVMEDIA_TYPE_VIDEO);

		if (flag <= 0)
		{
			stream_mapping[i] = -1;
			continue;
		}

		stream_mapping[i] = stream_index++;

		out_stream = avformat_new_stream(ofmt_ctx, NULL);

		if (!out_stream)
		{
			av_log(NULL, AV_LOG_ERROR, "Can`t alloc output stream\n");
			return -1;
		}

		//将 输入流 的参数拷贝到 输出流 中
		if ((err_code = avcodec_parameters_copy(out_stream->codecpar, in_codecpar)) < 0)
		{
			av_log(NULL, AV_LOG_ERROR, "Can`t copy output stream\n");
			return -1;
		}
		out_stream->codecpar->codec_tag = 0;
	}

	//输出 输出文件的信息
	av_dump_format(ofmt_ctx, 0, dst, 1);

	if (!(ofmt->flags & AVFMT_NOFILE))
	{
		//创建并初始化一个AVIOContext，用以访问dst目录的指定的资源
		if ((err_code = avio_open(&ofmt_ctx->pb, dst, AVIO_FLAG_WRITE)) < 0)
		{
			av_log(NULL, AV_LOG_ERROR, "Can`t open outfile\n");
			return -1;
		}
	}

	//写入文件头
	if ((err_code = avformat_write_header(ofmt_ctx, NULL)) < 0)
	{
		av_log(NULL, AV_LOG_ERROR, "Can`t write header\n");
		return -1;
	}

	av_init_packet(&pkt);

	printf("%d\n", &ofmt_ctx->nb_streams);

	while (1)
	{
		AVStream *in_stream, *out_stream;

		//循环读入输入文件的packet
		if ((err_code = av_read_frame(ifmt_ctx, &pkt)) < 0)
		{
			break;
		}

		//通过pkt下标获取当前访问的流
		in_stream = ifmt_ctx->streams[pkt.stream_index];
		if (pkt.stream_index >= stream_mapping_size || stream_mapping[pkt.stream_index] == -1)
		{
			av_packet_unref(&pkt);
			continue;
		}

		//重写压缩过流（音视字）的pkt序号
		pkt.stream_index = stream_mapping[pkt.stream_index];
		//通过重写下标找到输出上下文的中的对应流（要转换格式）
		out_stream = ofmt_ctx->streams[pkt.stream_index];
		//log_packet()

		// 关于AVStream.time_base的说明：
		// 输入：输入流中含有time_base，在avformat_find_stream_info()中可取到每个流中的time_base
		// 输出：avformat_write_header()会根据输出的封装格式确定每个流的time_base并写入文件中
		// AVPacket.pts和AVPacket.dts的单位是AVStream.time_base，不同的封装格式其AVStream.time_base不同
		// 所以输出文件中，每个packet需要根据输出封装格式重新计算pts和dts

		//重写pts dts
		// pkt.pts = av_rescale_q_rnd(pkt.pts, in_stream->time_base, out_stream->time_base,(AVRounding)( AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX) );
		// pkt.dts = av_rescale_q_rnd(pkt.dts, in_stream->time_base, out_stream->time_base,(AVRounding) (AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX) );
		// pkt.duration = av_rescale_q(pkt.duration, in_stream->time_base, out_stream->time_base);

		// 将packet中的各时间值从输入流封装格式时间基转换到输出流封装格式时间基
		av_packet_rescale_ts(&pkt, in_stream->time_base, out_stream->time_base);

		pkt.pos = -1;

		//将packet 写入输出
		if ((err_code = av_interleaved_write_frame(ofmt_ctx, &pkt)) < 0)
		{
			av_log(NULL, AV_LOG_ERROR, "Can`t write packet\n");
			return -1;
		}

		av_packet_unref(&pkt);
	}

	//写尾部
	av_write_trailer(ofmt_ctx);

	avformat_close_input(&ifmt_ctx);

	if (ofmt_ctx && !(ofmt->flags & AVFMT_NOFILE))
		avio_closep(&ofmt_ctx->pb);
	avformat_free_context(ofmt_ctx);

	av_freep(&stream_mapping);

	return 0;
}
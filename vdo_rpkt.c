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
//STR string ��str���ֵĴ�С
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

	//ע��������� �°�ffmpeg�Ѿ�����Ҫ��ǰע����
	//av_register_all();

	//������־����
	av_log_set_level(AV_LOG_INFO);

	//���������ʽ������
	AVOutputFormat *ofmt = NULL;
	//���������ļ�������ļ��ĸ�ʽ������
	AVFormatContext *ifmt_ctx = NULL, *ofmt_ctx = NULL;
	AVPacket pkt;

	//�ļ���ȡ
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

	//�������ļ� ����ֵ��������ifmt_ctx
	if ((err_code = avformat_open_input(&ifmt_ctx, src, NULL, NULL)) < 0)
	{
		av_strerror(err_code, errinfo, 1024);
		av_log(NULL, AV_LOG_ERROR, "Can`t open file! ( %s )\n", errinfo);
		return -1;
	}

	//Ѱ������Ϣ ȷ������
	if ((err_code = avformat_find_stream_info(ifmt_ctx, NULL)) < 0)
	{
		av_strerror(err_code, errinfo, 1024);
		av_log(NULL, AV_LOG_ERROR, "Can`t find stream in formation ( %s )\n", errinfo);
		return -1;
	}

	//��������ļ���Ϣ
	av_dump_format(ifmt_ctx, 0, src, 0);

	//������������ģ�ͬʱ����dst�Զ�ʶ���׺�����ļ�����
	avformat_alloc_output_context2(&ofmt_ctx, NULL, NULL, dst);

	if (!ofmt_ctx)
	{
		av_log(NULL, AV_LOG_ERROR, "Can`t create output context!\n");
		return -1;
	}

	//��ʼ��ӳ�������size ��¼��Щ������Ҫ��
	stream_mapping_size = ifmt_ctx->nb_streams;
	stream_mapping = (int *)(av_malloc_array(stream_mapping_size, sizeof(*stream_mapping)));
	int stream_index = 0;

	if (!stream_mapping)
	{
		av_log(NULL, AV_LOG_ERROR, "can`t molloc to streammapping\n");
		return -1;
	}

	//��ofmt����Ϊ����ļ���������ʽ
	ofmt = ofmt_ctx->oformat;

	for (int i = 0; i < ifmt_ctx->nb_streams; ++i)
	{
		AVStream *out_stream = NULL;
		AVStream *in_stream = ifmt_ctx->streams[i];			  //��ȡ��Ӧ�����ļ�������Ϣ
		AVCodecParameters *in_codecpar = in_stream->codecpar; //��ȡ��ǰ��������������

		//���˷���Ƶ ��Ļ ��Ƶ������Ϣ
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

		//�� ������ �Ĳ��������� ����� ��
		if ((err_code = avcodec_parameters_copy(out_stream->codecpar, in_codecpar)) < 0)
		{
			av_log(NULL, AV_LOG_ERROR, "Can`t copy output stream\n");
			return -1;
		}
		out_stream->codecpar->codec_tag = 0;
	}

	//��� ����ļ�����Ϣ
	av_dump_format(ofmt_ctx, 0, dst, 1);

	if (!(ofmt->flags & AVFMT_NOFILE))
	{
		//��������ʼ��һ��AVIOContext�����Է���dstĿ¼��ָ������Դ
		if ((err_code = avio_open(&ofmt_ctx->pb, dst, AVIO_FLAG_WRITE)) < 0)
		{
			av_log(NULL, AV_LOG_ERROR, "Can`t open outfile\n");
			return -1;
		}
	}

	//д���ļ�ͷ
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

		//ѭ�����������ļ���packet
		if ((err_code = av_read_frame(ifmt_ctx, &pkt)) < 0)
		{
			break;
		}

		//ͨ��pkt�±��ȡ��ǰ���ʵ���
		in_stream = ifmt_ctx->streams[pkt.stream_index];
		if (pkt.stream_index >= stream_mapping_size || stream_mapping[pkt.stream_index] == -1)
		{
			av_packet_unref(&pkt);
			continue;
		}

		//��дѹ�������������֣���pkt���
		pkt.stream_index = stream_mapping[pkt.stream_index];
		//ͨ����д�±��ҵ���������ĵ��еĶ�Ӧ����Ҫת����ʽ��
		out_stream = ofmt_ctx->streams[pkt.stream_index];
		//log_packet()

		// ����AVStream.time_base��˵����
		// ���룺�������к���time_base����avformat_find_stream_info()�п�ȡ��ÿ�����е�time_base
		// �����avformat_write_header()���������ķ�װ��ʽȷ��ÿ������time_base��д���ļ���
		// AVPacket.pts��AVPacket.dts�ĵ�λ��AVStream.time_base����ͬ�ķ�װ��ʽ��AVStream.time_base��ͬ
		// ��������ļ��У�ÿ��packet��Ҫ���������װ��ʽ���¼���pts��dts

		//��дpts dts
		// pkt.pts = av_rescale_q_rnd(pkt.pts, in_stream->time_base, out_stream->time_base,(AVRounding)( AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX) );
		// pkt.dts = av_rescale_q_rnd(pkt.dts, in_stream->time_base, out_stream->time_base,(AVRounding) (AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX) );
		// pkt.duration = av_rescale_q(pkt.duration, in_stream->time_base, out_stream->time_base);

		// ��packet�еĸ�ʱ��ֵ����������װ��ʽʱ���ת�����������װ��ʽʱ���
		av_packet_rescale_ts(&pkt, in_stream->time_base, out_stream->time_base);

		pkt.pos = -1;

		//��packet д�����
		if ((err_code = av_interleaved_write_frame(ofmt_ctx, &pkt)) < 0)
		{
			av_log(NULL, AV_LOG_ERROR, "Can`t write packet\n");
			return -1;
		}

		av_packet_unref(&pkt);
	}

	//дβ��
	av_write_trailer(ofmt_ctx);

	avformat_close_input(&ifmt_ctx);

	if (ofmt_ctx && !(ofmt->flags & AVFMT_NOFILE))
		avio_closep(&ofmt_ctx->pb);
	avformat_free_context(ofmt_ctx);

	av_freep(&stream_mapping);

	return 0;
}
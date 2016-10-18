/*****************************************************************************
 * SDIStream.hpp: SDI sout module for vlc
 *****************************************************************************
 * Copyright © 2018 VideoLabs, VideoLAN and VideoLAN Authors
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/
#ifndef SDISTREAM_HPP
#define SDISTREAM_HPP

#include <vlc_common.h>
#include <vlc_filter.h>
#include <vlc_aout.h>
#include <queue>
#include <mutex>

namespace sdi_sout
{
    class AbstractStreamOutputBuffer
    {
        public:
            AbstractStreamOutputBuffer();
            virtual ~AbstractStreamOutputBuffer();
            virtual void FlushQueued() = 0;
            void Enqueue(void *);
            void * Dequeue();

        private:
            std::mutex queue_mutex;
            std::queue<void *> queued;
    };

    class BlockStreamOutputBuffer : public AbstractStreamOutputBuffer
    {
        public:
            BlockStreamOutputBuffer();
            virtual ~BlockStreamOutputBuffer();
            virtual void FlushQueued();
    };

    class PictureStreamOutputBuffer : public AbstractStreamOutputBuffer
    {
        public:
            PictureStreamOutputBuffer();
            virtual ~PictureStreamOutputBuffer();
            virtual void FlushQueued();
    };

    class StreamID
    {
        public:
            StreamID(int);
            StreamID(int, int);
            StreamID& operator=(const StreamID &);
            bool      operator==(const StreamID &);
            std::string toString() const;

        private:
            int stream_id;
            unsigned sequence_id;
            static unsigned i_next_sequence_id;
    };

    class AbstractStream
    {
        public:
            AbstractStream(vlc_object_t *, const StreamID &,
                           AbstractStreamOutputBuffer *);
            virtual ~AbstractStream();
            virtual bool init(const es_format_t *) = 0;
            virtual int Send(block_t*) = 0;
            virtual void Drain() = 0;
            virtual void Flush() = 0;
            const StreamID & getID() const;

        protected:
            vlc_object_t *p_stream;
            AbstractStreamOutputBuffer *outputbuffer;

        private:
            StreamID id;
    };

    class AbstractDecodedStream : public AbstractStream
    {
        public:
            AbstractDecodedStream(vlc_object_t *, const StreamID &,
                                  AbstractStreamOutputBuffer *);
            virtual ~AbstractDecodedStream();
            virtual bool init(const es_format_t *); /* impl */
            virtual int Send(block_t*);
            virtual void Flush();
            virtual void Drain();
            void setOutputFormat(const es_format_t *);

        protected:
            decoder_t *p_decoder;
            virtual void setCallbacks() = 0;
            es_format_t requestedoutput;
    };

    class VideoDecodedStream : public AbstractDecodedStream
    {
        public:
            VideoDecodedStream(vlc_object_t *, const StreamID &,
                               AbstractStreamOutputBuffer *);
            virtual ~VideoDecodedStream();
            virtual void setCallbacks();

        private:
            static void VideoDecCallback_queue(decoder_t *, picture_t *);
            static int VideoDecCallback_update_format(decoder_t *);
            static picture_t *VideoDecCallback_new_buffer(decoder_t *);
            filter_chain_t * VideoFilterCreate(const es_format_t *);
            void Output(picture_t *);
            filter_chain_t *p_filters_chain;
    };

#   define FRAME_SIZE 1920
    class AudioDecodedStream : public AbstractDecodedStream
    {
        public:
            AudioDecodedStream(vlc_object_t *, const StreamID &,
                               AbstractStreamOutputBuffer *);
            virtual ~AudioDecodedStream();
            virtual void setCallbacks();

        private:
            static void AudioDecCallback_queue(decoder_t *, block_t *);
            static int AudioDecCallback_update_format(decoder_t *);
            aout_filters_t *AudioFiltersCreate(const es_format_t *);
            void Output(block_t *);
            aout_filters_t *p_filters;
    };
}

#endif

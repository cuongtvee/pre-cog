/*
 * Copyright 2012 Free Software Foundation, Inc.
 * 
 * This file is part of GNU Radio
 * 
 * GNU Radio is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 * 
 * GNU Radio is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with GNU Radio; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */
#ifndef INCLUDED_GR_EXTRAS_BLOCK_H
#define INCLUDED_GR_EXTRAS_BLOCK_H

#include <gnuradio/extras/api.h>
#include <gruel/pmt_extras.h>
#include <gr_hier_block2.h>
#include <gr_block.h>

namespace gnuradio{ namespace extras{

template <typename PtrType> struct Buffer
{
    //! get a native pointer type to this buffer
    inline PtrType get(void) const
    {
        return _mem;
    }

    //! get a pointer of the desired type to this buffer
    template <typename T> inline T cast(void) const
    {
        return reinterpret_cast<T>(this->get());
    }

    //! get the number of items in this buffer
    inline size_t size(void) const
    {
        return _len;
    }

//private:
    PtrType _mem;
    size_t _len;
};

//! Message signature describes the inputs and outputs of message passing
struct GR_EXTRAS_API msg_signature
{
    msg_signature(const bool has_input = false, const size_t num_outputs = 0):
        has_input(has_input), num_outputs(num_outputs)
    {
        //NOP
    }

    bool has_input;
    size_t num_outputs;
};

/*!
 * The base clock class that provides message passing,
 * and a more object oriented access to work buffers.
 * This block implements basic and sync block functionalities.
 */
class GR_EXTRAS_API block : public gr_hier_block2{
public:

    //! empty constructor for virtual inheritance
    block(void){}

    /*!
     * The block constructor.
     * The message output ports creates optional output ports,
     * considered to be indexed after the output IO signature.
     * Additionaly, there is 1 optional input message port,
     * considered to be indexed after the input IO signature.
     * \param name the name of this block
     * \param in_sig the input signature
     * \param out_sig the output signature
     * \param msg_sig the IO for msg passing
     */
    block(
        const std::string &name,
        gr_io_signature_sptr in_sig,
        gr_io_signature_sptr out_sig,
        const msg_signature &msg_sig = msg_signature()
    );

    //! deconstructor
    virtual ~block(void);

    /*!
     * Set the block's work mode (how it produces and consumes, and the ratio).
     * When automatic, consume is automatically called, and forecast handled.
     * \param automatic true to call consume and forecast automatically
     */
    void set_auto(const bool automatic);

    /*******************************************************************
     * Basic routines from basic block
     ******************************************************************/

    long unique_id(void) const;

    std::string name(void) const;

    unsigned history(void) const;

    void set_history(unsigned history);

    void set_output_multiple(int multiple);

    int output_multiple(void) const;

    void consume(int which_input, int how_many_items);

    void consume_each(int how_many_items);

    void produce(int which_output, int how_many_items);

    /*!
     * The relative rate can be thought of as interpolation/decimation.
     * In other words, relative rate is the ratio of output items to input items.
     */
    void set_relative_rate(double relative_rate);

    double relative_rate(void) const;

    /*******************************************************************
     * Tag related routines from basic block
     ******************************************************************/

    uint64_t nitems_read(unsigned int which_input);

    uint64_t nitems_written(unsigned int which_output);

    gr_block::tag_propagation_policy_t tag_propagation_policy(void);

    void set_tag_propagation_policy(gr_block::tag_propagation_policy_t p);

    void add_item_tag(
        unsigned int which_output, const gr_tag_t &tag
    );

    void add_item_tag(
        unsigned int which_output,
        uint64_t abs_offset,
        const pmt::pmt_t &key,
        const pmt::pmt_t &value,
        const pmt::pmt_t &srcid=pmt::PMT_F
    );

    void get_tags_in_range(
        std::vector<gr_tag_t> &tags,
        unsigned int which_input,
        uint64_t abs_start,
        uint64_t abs_end
    );

    void get_tags_in_range(
        std::vector<gr_tag_t> &tags,
        unsigned int which_input,
        uint64_t abs_start,
        uint64_t abs_end,
        const pmt::pmt_t &key
    );

    /*******************************************************************
     * Message passing related routines from basic block
     ******************************************************************/
    /*!
     * \brief Check if a message is available to pop.
     * \return true is a message is in the queue
     */
    bool check_msg_queue(void);

    /*!
     * \brief Pop a message from the front of the queue.
     * This function will block until a message is available.
     * \return the message as a tag type
     */
    gr_tag_t pop_msg_queue(void);

    /*!
     * \brief Post a message to a subscriber group.
     * All message subscribers in the group will get this message.
     * \param group the index of the subscriber group
     * \param msg the message to post to all subscribers
     */
    void post_msg(const size_t &group, const gr_tag_t &msg);

    /*!
     * \brief Post a message to a subscriber group.
     *
     * \param group the index of the subscriber group
     * \param key the tag key as a PMT symbol
     * \param value any PMT holding any value for the given key
     * \param srcid optional source ID specifier; defaults to PMT_F
     */
    inline void post_msg(
        const size_t &group,
        const pmt::pmt_t &key,
        const pmt::pmt_t &value,
        const pmt::pmt_t &srcid=pmt::PMT_F
    ){
        gr_tag_t tag;
        tag.offset = 0; //not used
        tag.key = key;
        tag.value = value;
        tag.srcid = srcid;
        this->post_msg(group, tag);
    }

    /*******************************************************************
     * Work related routines from basic block
     ******************************************************************/

    //! Called when the flow graph is started, can overload
    virtual bool start(void);

    //! Called when the flow graph is stopped, can overload
    virtual bool stop(void);

    typedef std::vector<Buffer<const void *> > InputItems;
    typedef std::vector<Buffer<void *> > OutputItems;

    //! The official call into the work routine (overload please)
    virtual int work(
        const InputItems &input_items,
        const OutputItems &output_items
    ) = 0;

    //! forcast requirements, can be overloaded
    virtual void forecast(
        int noutput_items,
        gr_vector_int &ninput_items_required
    );

private:
    //forward declared private guts
    struct impl;
    boost::shared_ptr<impl> _impl;
};

}}

#endif /* INCLUDED_GR_EXTRAS_BLOCK_H */

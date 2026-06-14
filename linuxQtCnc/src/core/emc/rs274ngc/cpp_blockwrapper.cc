/********************************************************************
* Description: cpp_blockwrapper.cc
*   C++ Block Wrapper Class Implementation for linuxQtCnc
*
*   This file implements the pure C++ replacement for Python block wrapper.
*
*   Derived from a work by Michael Haberler
*
* Author: linuxQtCnc project
* License: GPL Version 2
* System: Linux / Windows
*
* Copyright (c) 2026 All rights reserved.
********************************************************************/

#include "cpp_blockwrapper.hh"
#include "interp_internal.hh"

#include <algorithm>

namespace InterpBlocks {

// ========================================================================
// BlockWrapper Implementation
// ========================================================================

BlockWrapper::BlockWrapper(block_struct* block)
    : m_block(block)
{
}

BlockWrapper::BlockWrapper(const BlockWrapper& other)
    : m_block(other.m_block)
{
}

BlockWrapper& BlockWrapper::operator=(const BlockWrapper& other)
{
    m_block = other.m_block;
    return *this;
}

BlockWrapper::~BlockWrapper()
{
    // We don't own the block, just hold a pointer
}

// ---- G Modes ----

int BlockWrapper::getGMode(int group) const
{
    if (!m_block || group < 0 || group >= GM_MAX_MODAL_GROUPS) {
        return -1;
    }
    return m_block->g_modes[group];
}

std::vector<int> BlockWrapper::getAllGModes() const
{
    std::vector<int> modes;
    if (!m_block) return modes;

    for (int i = 0; i < GM_MAX_MODAL_GROUPS; i++) {
        modes.push_back(m_block->g_modes[i]);
    }
    return modes;
}

void BlockWrapper::setGMode(int group, int mode)
{
    if (!m_block || group < 0 || group >= GM_MAX_MODAL_GROUPS) {
        return;
    }
    m_block->g_modes[group] = mode;
}

// ---- M Modes ----

int BlockWrapper::getMMode(int group) const
{
    if (!m_block || group < 0 || group >= MAX_EMS) {
        return -1;
    }
    return m_block->m_modes[group];
}

std::vector<int> BlockWrapper::getAllMModes() const
{
    std::vector<int> modes;
    if (!m_block) return modes;

    for (int i = 0; i < MAX_EMS; i++) {
        modes.push_back(m_block->m_modes[i]);
    }
    return modes;
}

void BlockWrapper::setMMode(int group, int mode)
{
    if (!m_block || group < 0 || group >= MAX_EMS) {
        return;
    }
    m_block->m_modes[group] = mode;
}

// ---- Parameters ----

double BlockWrapper::getParam(int index) const
{
    if (!m_block || index < 0 || index >= INTERP_SUB_PARAMS) {
        return 0.0;
    }
    return m_block->params[index];
}

std::vector<double> BlockWrapper::getAllParams() const
{
    std::vector<double> params;
    if (!m_block) return params;

    for (int i = 0; i < INTERP_SUB_PARAMS; i++) {
        params.push_back(m_block->params[i]);
    }
    return params;
}

void BlockWrapper::setParam(int index, double value)
{
    if (!m_block || index < 0 || index >= INTERP_SUB_PARAMS) {
        return;
    }
    m_block->params[index] = value;
}

// ---- Word Flags ----

bool BlockWrapper::hasX() const { return m_block && m_block->x_flag; }
bool BlockWrapper::hasY() const { return m_block && m_block->y_flag; }
bool BlockWrapper::hasZ() const { return m_block && m_block->z_flag; }
bool BlockWrapper::hasA() const { return m_block && m_block->a_flag; }
bool BlockWrapper::hasB() const { return m_block && m_block->b_flag; }
bool BlockWrapper::hasC() const { return m_block && m_block->c_flag; }
bool BlockWrapper::hasF() const { return m_block && m_block->f_flag; }
bool BlockWrapper::hasS() const { return m_block && m_block->s_flag; }
bool BlockWrapper::hasT() const { return m_block && m_block->t_flag; }
bool BlockWrapper::hasP() const { return m_block && m_block->p_flag; }
bool BlockWrapper::hasQ() const { return m_block && m_block->q_flag; }
bool BlockWrapper::hasR() const { return m_block && m_block->r_flag; }
bool BlockWrapper::hasI() const { return m_block && m_block->i_flag; }
bool BlockWrapper::hasJ() const { return m_block && m_block->j_flag; }
bool BlockWrapper::hasK() const { return m_block && m_block->k_flag; }
bool BlockWrapper::hasL() const { return m_block && m_block->l_flag; }
bool BlockWrapper::hasH() const { return m_block && m_block->h_flag; }
bool BlockWrapper::hasD() const { return m_block && m_block->d_flag; }
bool BlockWrapper::hasE() const { return m_block && m_block->e_flag; }
bool BlockWrapper::hasU() const { return m_block && m_block->u_flag; }
bool BlockWrapper::hasV() const { return m_block && m_block->v_flag; }
bool BlockWrapper::hasW() const { return m_block && m_block->w_flag; }

// ---- Word Values ----

double BlockWrapper::getX() const { return m_block ? m_block->x_number : 0.0; }
double BlockWrapper::getY() const { return m_block ? m_block->y_number : 0.0; }
double BlockWrapper::getZ() const { return m_block ? m_block->z_number : 0.0; }
double BlockWrapper::getA() const { return m_block ? m_block->a_number : 0.0; }
double BlockWrapper::getB() const { return m_block ? m_block->b_number : 0.0; }
double BlockWrapper::getC() const { return m_block ? m_block->c_number : 0.0; }
double BlockWrapper::getF() const { return m_block ? m_block->f_number : 0.0; }
double BlockWrapper::getS() const { return m_block ? m_block->s_number : 0.0; }
int    BlockWrapper::getT() const { return m_block ? m_block->t_number : 0; }
double BlockWrapper::getP() const { return m_block ? m_block->p_number : 0.0; }
double BlockWrapper::getQ() const { return m_block ? m_block->q_number : 0.0; }
double BlockWrapper::getR() const { return m_block ? m_block->r_number : 0.0; }
double BlockWrapper::getI() const { return m_block ? m_block->i_number : 0.0; }
double BlockWrapper::getJ() const { return m_block ? m_block->j_number : 0.0; }
double BlockWrapper::getK() const { return m_block ? m_block->k_number : 0.0; }
int    BlockWrapper::getL() const { return m_block ? m_block->l_number : 0; }
int    BlockWrapper::getH() const { return m_block ? m_block->h_number : 0; }
double BlockWrapper::getD() const { return m_block ? m_block->d_number_float : 0.0; }
double BlockWrapper::getE() const { return m_block ? m_block->e_number : 0.0; }
double BlockWrapper::getU() const { return m_block ? m_block->u_number : 0.0; }
double BlockWrapper::getV() const { return m_block ? m_block->v_number : 0.0; }
double BlockWrapper::getW() const { return m_block ? m_block->w_number : 0.0; }

// ---- Comment and O-word ----

std::string BlockWrapper::getComment() const
{
    if (!m_block) return "";
    return std::string(m_block->comment);
}

void BlockWrapper::setComment(const std::string& comment)
{
    if (!m_block) return;
    rtapi_strxcpy(m_block->comment, comment.c_str());
}

std::string BlockWrapper::getOName() const
{
    if (!m_block) return "";
    return std::string(m_block->o_name);
}

void BlockWrapper::setOName(const std::string& name)
{
    if (!m_block) return;
    rtapi_strxcpy(m_block->o_name, name.c_str());
}

// ---- Line Number ----

int BlockWrapper::getLineNumber() const
{
    return m_block ? m_block->line_number : 0;
}

void BlockWrapper::setLineNumber(int line)
{
    if (m_block) m_block->line_number = line;
}

// ---- Sequence Number ----

int BlockWrapper::getSequenceNumber() const
{
    return m_block ? m_block->n_number : 0;
}

// ---- Tool Information ----

int BlockWrapper::getToolNumber() const
{
    return m_block ? m_block->t_number : 0;
}

int BlockWrapper::getPocketNumber() const
{
    // Pocket number is stored in Q word for some tool change operations
    return m_block ? static_cast<int>(m_block->q_number) : 0;
}

// ---- Motion Type ----

int BlockWrapper::getMotionType() const
{
    // Motion type is determined by G modes
    // G0 = rapid, G1 = linear, G2/G3 = arc
    if (!m_block) return -1;

    int motion_mode = m_block->g_modes[GM_MODAL_GROUP_1];
    return motion_mode;
}

// ---- Arc Parameters ----

double BlockWrapper::getArcCenterX() const { return getI(); }
double BlockWrapper::getArcCenterY() const { return getJ(); }
double BlockWrapper::getArcCenterZ() const { return getK(); }

double BlockWrapper::getArcRadius() const
{
    // Arc radius is stored in R word for R-format arcs
    return getR();
}

// ---- Cutter Compensation ----

int BlockWrapper::getCutterCompDirection() const
{
    // Cutter compensation is determined by G41/G42
    if (!m_block) return 0;

    int cutter_mode = m_block->g_modes[GM_MODAL_GROUP_7];
    if (cutter_mode == G_41) return 1;   // Left
    if (cutter_mode == G_42) return 2;   // Right
    return 0;  // Off
}

// ---- Cycle Parameters ----

double BlockWrapper::getCycleRetract() const
{
    // Retract position for canned cycles is in R word
    return getR();
}

double BlockWrapper::getCycleDepth() const
{
    // Depth for canned cycles is in Z word
    return getZ();
}

// ========================================================================
// BlockArrayWrapper Implementation
// ========================================================================

BlockArrayWrapper::BlockArrayWrapper(block_struct* blocks, int size)
    : m_blocks(blocks)
    , m_size(size)
{
}

BlockArrayWrapper::~BlockArrayWrapper()
{
    // We don't own the blocks array
}

BlockWrapper BlockArrayWrapper::getBlock(int index) const
{
    if (!m_blocks || index < 0 || index >= m_size) {
        return BlockWrapper(nullptr);
    }
    return BlockWrapper(&m_blocks[index]);
}

} // namespace InterpBlocks
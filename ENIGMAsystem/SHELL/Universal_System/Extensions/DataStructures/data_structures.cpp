/********************************************************************************\
 **                                                                              **
 **  Copyright (C) 2011 Alasdair Morrison <tgmg@g-java.com>                      **
 **  Copyright (C) 2011 Polygone                                                 **
 **                                                                              **
 **  This file is a part of the ENIGMA Development Environment.                  **
 **                                                                              **
 **                                                                              **
 **  ENIGMA is free software: you can redistribute it and/or modify it under the **
 **  terms of the GNU General Public License as published by the Free Software   **
 **  Foundation, version 3 of the license or any later version.                  **
 **                                                                              **
 **  This application and its source code is distributed AS-IS, WITHOUT ANY      **
 **  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS   **
 **  FOR A PARTICULAR PURPOSE. See the GNU General Public License for more       **
 **  details.                                                                    **
 **                                                                              **
 **  You should have recieved a copy of the GNU General Public License along     **
 **  with this code. If not, see <http://www.gnu.org/licenses/>                  **
 **                                                                              **
 **  ENIGMA is an environment designed to create games and other programs with a **
 **  high-level, fully compilable language. Developers of ENIGMA or anything     **
 **  associated with ENIGMA are in no way responsible for its users or           **
 **  applications created by its users, or damages caused by the environment     **
 **  or programs made in the environment.                                        **
 **                                                                              **
 \********************************************************************************/

#include <float.h>
#include <random>
#include <algorithm>
#include <iterator>
#include <map>
#include <limits>
#include <deque>
#include <vector>

#include <sstream>
#include <string>

#include <floatcomp.h>

using namespace std;

#include "include.h"

template<typename T> static inline T maxv(T a, T b) { return (a > b) ? a : b; }
template<typename T> static inline T minv(T a, T b) { return (a < b) ? a : b; }

template<typename t> bool tequal(t v1, t v2) { return v1 == v2; }
template<> bool tequal(float v1, float v2)   { return fequal(v1, v2); }
template<> bool tequal(double v1, double v2) { return fequal(v1, v2); }

template<class RandomIt>
void mt_random_shuffle(RandomIt first, RandomIt last) {
  std::random_device rd;
  std::mt19937 g(rd());
  std::shuffle(first, last, g);
}

template <typename t>
class grid
{
    unsigned int xgrid, ygrid;
    t *grid_array;

    public:
    grid() : xgrid(0), ygrid(0), grid_array(nullptr) {}
    grid(const unsigned int w, const unsigned int h) {
      ygrid = h;
      xgrid = w;
      if (h != 0 && w > std::numeric_limits<size_t>::max() / h)
        throw std::bad_array_new_length();
      grid_array = new t[static_cast<size_t>(w) * h];
    }
    ~grid() : xgrid(0), ygrid(0), grid_array(nullptr) {}

    void destroy()
    {
        delete[] grid_array;
        grid_array = nullptr;
        xgrid = 0;
        ygrid = 0;
    }
    void clear()
    {
        if (ygrid != 0 &&
            xgrid > std::numeric_limits<size_t>::max() / ygrid)
            throw std::bad_array_new_length();

        const size_t count = static_cast<size_t>(xgrid) * ygrid;
        t *new_array = new t[count];

        delete[] grid_array;
        grid_array = new_array;
    }
    void resize(unsigned w, unsigned h)
    {
        t *new_grid_array = new t[w * h];
        const unsigned int wm = minv(xgrid, w), hm = minv(ygrid, h);
        for (unsigned i = 0; i < hm; i++)
            for (unsigned ii = 0; ii < wm; ii++)
                new_grid_array[i * w + ii] = grid_array[i * xgrid + ii];

        delete[] grid_array;

        grid_array = new_grid_array;
        xgrid = w;
        ygrid = h;
    }
    void copy(const grid& copy_id)
    {
        if (this == &copy_id)
            return;

        delete[] grid_array;
        if (copy_id.ygrid != 0 &&
            copy_id.xgrid > std::numeric_limits<size_t>::max() / copy_id.ygrid)
            throw std::bad_array_new_length();

        const size_t count =
            static_cast<size_t>(copy_id.ygrid) * copy_id.xgrid;
        t *new_array = new t[count];

        delete[] grid_array;
        grid_array = new_array;
        xgrid = copy_id.xgrid;
        ygrid = copy_id.ygrid;
        for (unsigned i = 0; i < ygrid; i++)
            for (unsigned ii = 0; ii < xgrid; ii++)
                grid_array[i * xgrid + ii] = copy_id.grid_array[i * copy_id.xgrid + ii];
    }
    unsigned int width()
    {
        return xgrid;
    }
    unsigned int height()
    {
        return ygrid;
    }
    void insert(const unsigned int x, const unsigned int y, const t val)
    {
        if (x < xgrid && y < ygrid)
            grid_array[y * xgrid + x] = val;
    }
    void add(const unsigned int x, const unsigned int y, const t val)
    {
        if (x < xgrid && y < ygrid)
            grid_array[y * xgrid + x] += val;
    }
    void multiply(const unsigned int x, const unsigned int y, const double val)
    {
        if (x < xgrid && y < ygrid)
            grid_array[y * xgrid + x] *= val;
    }
    void insert_region(const unsigned int x1, const unsigned int y1, unsigned int x2, const unsigned int y2, const t val)
    {
       const int tx1 = minv(x1, x2),  ty1 = minv(y1, y2), tx2 = maxv(x1, x2), ty2 = maxv(y1, y2), xd = xgrid - tx1, yd = ygrid - ty1;
       if (xd > 0 && yd > 0)
       {
       const unsigned int tx1 = minv(x1, x2);
       const unsigned int ty1 = minv(y1, y2);
       const unsigned int tx2 = maxv(x1, x2);
       const unsigned int ty2 = maxv(y1, y2);

       if (tx1 >= xgrid || ty1 >= ygrid)
           return;

       const unsigned int px2 = tx2 >= xgrid - 1 ? xgrid : tx2 + 1;
       const unsigned int py2 = ty2 >= ygrid - 1 ? ygrid : ty2 + 1;

       for (unsigned int i = ty1; i < py2; i++)
           for (unsigned int ii = tx1; ii < px2; ii++)
                   grid_array[i * xgrid + ii] = val;
    }
    void add_region(const unsigned int x1, const unsigned int y1, unsigned int x2, const unsigned int y2, const t val)
    {
       const unsigned int tx1 = minv(x1, x2);
       const unsigned int ty1 = minv(y1, y2);
       const unsigned int tx2 = maxv(x1, x2);
       const unsigned int ty2 = maxv(y1, y2);

       if (tx1 >= xgrid || ty1 >= ygrid)
           return;

       const unsigned int px2 = tx2 >= xgrid - 1 ? xgrid : tx2 + 1;
       const unsigned int py2 = ty2 >= ygrid - 1 ? ygrid : ty2 + 1;

       for (unsigned int i = ty1; i < py2; i++)
           for (unsigned int ii = tx1; ii < px2; ii++)
                   grid_array[i * xgrid + ii] += val;
    }
    void multiply_region(const unsigned int x1, const unsigned int y1, unsigned int x2, const unsigned int y2, const double val)
    {
       const unsigned int tx1 = minv(x1, x2);
       const unsigned int ty1 = minv(y1, y2);
       const unsigned int tx2 = maxv(x1, x2);
       const unsigned int ty2 = maxv(y1, y2);

       if (tx1 >= xgrid || ty1 >= ygrid)
           return;

       const unsigned int px2 = tx2 >= xgrid - 1 ? xgrid : tx2 + 1;
       const unsigned int py2 = ty2 >= ygrid - 1 ? ygrid : ty2 + 1;

       for (unsigned int i = ty1; i < py2; i++)
           for (unsigned int ii = tx1; ii < px2; ii++)
                   grid_array[i * xgrid + ii] *= val;
    }
    void insert_disk(const double x, const double y, const double r, const t val)
    {
        const double rr = r*r;
        const int tx1 = int(x - r), ty1 = int(y - r), tx2 = int(x + r + 1), ty2 = int(y + r + 1);
        if (tx2 >= 0 && ty2 >=0 && tx1 < int(xgrid) && ty1 < int(ygrid))
        {
            const int px1 = maxv(tx1, 0), py1 = maxv(ty1, 0), px2 = minv(tx2, (int)xgrid), py2 = minv(ty2, (int)ygrid);
            for (int i = py1; i < py2; i++)
                for (int ii = px1; ii < px2; ii++)
                    if ((x - ii)*(x - ii) + (y - i)*(y - i) <= rr)
                        grid_array[i * xgrid + ii] = val;
        }
    }
    void add_disk(const double x, const double y, const double r, const t val)
    {
        const double rr = r*r;
        const int tx1 = int(x - r), ty1 = int(y - r), tx2 = int(x + r + 1), ty2 = int(y + r + 1);
        if (tx2 >= 0 && ty2 >=0 && tx1 < int(xgrid) && ty1 < int(ygrid))
        {
            const int px1 = maxv(tx1, 0), py1 = maxv(ty1, 0), px2 = minv(tx2, (int)xgrid), py2 = minv(ty2, (int)ygrid);
            for (int i = py1; i < py2; i++)
                for (int ii = px1; ii < px2; ii++)
                    if ((x - ii)*(x - ii) + (y - i)*(y - i) <= rr)
                        grid_array[i * xgrid + ii] += val;
        }
    }
    void multiply_disk(const double x, const double y, const double r, const double val)
    {
        const double rr = r*r;
        const int tx1 = int(x - r), ty1 = int(y - r), tx2 = int(x + r + 1), ty2 = int(y + r + 1);
        if (tx2 >= 0 && ty2 >=0 && tx1 < int(xgrid) && ty1 < int(ygrid))
        {
            const int px1 = maxv(tx1, 0), py1 = maxv(ty1, 0), px2 = minv(tx2, (int)xgrid), py2 = minv(ty2, (int)ygrid);
            for (int i = py1; i < py2; i++)
                for (int ii = px1; ii < px2; ii++)
                    if ((x - ii)*(x - ii) + (y - i)*(y - i) <= rr)
                        grid_array[i * xgrid + ii] *= val;
        }
    }
    void insert_grid_region(const grid& source_id, const unsigned int sx1, const unsigned int sy1, const unsigned int sx2, const unsigned int sy2, const unsigned int x, const unsigned int y)
    {
        if (x < xgrid && y < ygrid)
        {
            const int tx1 = minv(sx1, sx2),  ty1 = minv(sy1, sy2), tx2 = maxv(sx1, sx2), ty2 = maxv(sy1, sy2), xd = source_id.xgrid - tx1, yd = source_id.ygrid - ty1;
            if (xd > 0 && yd > 0)
            {
                const int upx = minv(tx2 - tx1 + 1, minv(int(xgrid - x), xd)), upy = minv(ty2 - ty1 + 1, minv(int(ygrid - y), yd));
                for (int i = 0; i < upy; i++)
                    for (int ii = 0; ii < upx; ii++)
                        grid_array[(y + i)*xgrid + (x + ii)] = source_id.grid_array[(ty1 + i)*source_id.xgrid + (tx1 + ii)];
            }
        }
    }
    void add_grid_region(const grid& source_id, const unsigned int sx1, const unsigned int sy1, const unsigned int sx2, const unsigned int sy2, const unsigned int x, const unsigned int y)
    {
        if (x < xgrid && y < ygrid)
        {
            const int tx1 = minv(sx1, sx2),  ty1 = minv(sy1, sy2), tx2 = maxv(sx1, sx2), ty2 = maxv(sy1, sy2), xd = source_id.xgrid - tx1, yd = source_id.ygrid - ty1;
            if (xd > 0 && yd > 0)
            {
                const int upx = minv(tx2 - tx1 + 1, minv(int(xgrid - x), xd)), upy = minv(ty2 - ty1 + 1, minv(int(ygrid - y), yd));
                for (int i = 0; i < upy; i++)
                    for (int ii = 0; ii < upx; ii++)
                        grid_array[(y + i)*xgrid + (x + ii)] += source_id.grid_array[(ty1 + i)*source_id.xgrid + (tx1 + ii)];
            }
        }
    }
    void multiply_grid_region(const grid& source_id, const unsigned int sx1, const unsigned int sy1, const unsigned int sx2, const unsigned int sy2, const unsigned int x, const unsigned int y)
    {
        if (x < xgrid && y < ygrid)
        {
            const int tx1 = minv(sx1, sx2),  ty1 = minv(sy1, sy2), tx2 = maxv(sx1, sx2), ty2 = maxv(sy1, sy2), xd = source_id.xgrid - tx1, yd = source_id.ygrid - ty1;
            if (xd > 0 && yd > 0)
            {
                const int upx = minv(tx2 - tx1 + 1, minv(int(xgrid - x), xd)), upy = minv(ty2 - ty1 + 1, minv(int(ygrid - y), yd));
                for (int i = 0; i < upy; i++)
                    for (int ii = 0; ii < upx; ii++)
                        grid_array[(y + i)*xgrid + (x + ii)] *= source_id.grid_array[(ty1 + i)*source_id.xgrid + (tx1 + ii)];
            }
        }
    }

    t find(unsigned int x, unsigned int y)
    {
        return (grid_array[y * xgrid + x]);
    }
    t find_region_sum(unsigned int x1, unsigned int y1, unsigned int x2, unsigned int y2)
    {
       const unsigned int tx1 = minv(x1, x2);
       const unsigned int ty1 = minv(y1, y2);
       const unsigned int tx2 = maxv(x1, x2);
       const unsigned int ty2 = maxv(y1, y2);
       if (tx1 < xgrid && ty1 < ygrid)
       {
           const unsigned int px2 = tx2 >= xgrid - 1 ? xgrid : tx2 + 1;
           const unsigned int py2 = ty2 >= ygrid - 1 ? ygrid : ty2 + 1;       
           variant sum = 0;
           for (unsigned int i = ty1; i < py2; i++)
               for (unsigned int ii = tx1; ii < px2; ii++)
                   sum += grid_array[i * xgrid + ii];
           return sum;
       }
       return t();
    }
    t find_region_max(unsigned int x1, unsigned int y1, unsigned int x2, unsigned int y2)
    {
       const int tx1 = minv(x1, x2),  ty1 = minv(y1, y2), tx2 = maxv(x1, x2), ty2 = maxv(y1, y2), xd = xgrid - tx1, yd = ygrid - ty1;
       t val_check;
       if (xd > 0 && yd > 0)
       {
           const int px1 = maxv(tx1, 0), py1 = maxv(ty1, 0), px2 = minv(tx2 + 1, (int)xgrid), py2 = minv(ty2 + 1, (int)ygrid);
           t max_check = grid_array[py1 * xgrid + px1];
           for (int i = py1; i < py2; i++)
               for (int ii = px1; ii < px2; ii++)
               {
                   val_check = grid_array[i * xgrid + ii];
                   if (val_check > max_check)
                       max_check = val_check;
               }
           return max_check;
       }
       return t();
    }
    t find_region_min(unsigned int x1, unsigned int y1, unsigned int x2, unsigned int y2)
    {
       const int tx1 = minv(x1, x2),  ty1 = minv(y1, y2), tx2 = maxv(x1, x2), ty2 = maxv(y1, y2), xd = xgrid - tx1, yd = ygrid - ty1;
       t val_check;
       if (xd > 0 && yd > 0)
       {
           const int px1 = maxv(tx1, 0), py1 = maxv(ty1, 0), px2 = minv(tx2 + 1, (int)xgrid), py2 = minv(ty2 + 1, (int)ygrid);
           t min_check = grid_array[py1 * xgrid + px1];
           for (int i = py1; i < py2; i++)
               for (int ii = px1; ii < px2; ii++)
               {
                   val_check = grid_array[i * xgrid + ii];
                   if (val_check < min_check)
                       min_check = val_check;
               }
           return min_check;
       }
       return t();
    }
    t find_region_mean(unsigned int x1, unsigned int y1, unsigned int x2, unsigned int y2)
    {
       const int tx1 = minv(x1, x2),  ty1 = minv(y1, y2), tx2 = maxv(x1, x2), ty2 = maxv(y1, y2), xd = xgrid - tx1, yd = ygrid - ty1;
       if (xd > 0 && yd > 0)
       {
           const int px1 = maxv(tx1, 0), py1 = maxv(ty1, 0), px2 = minv(tx2 + 1, (int)xgrid), py2 = minv(ty2 + 1, (int)ygrid);
           t sum = 0;
           for (int i = py1; i < py2; i++)
               for (int ii = px1; ii < px2; ii++)
                   sum += grid_array[i * xgrid + ii];
           const double region_size = (py2 - py1)*(px2 - px1);
           return sum/region_size;
       }
       return t();
    }
    t find_disk_sum(const double x, const double y, const double r)
    {
        const double rr = r*r;
        const int tx1 = int(x - r), ty1 = int(y - r), tx2 = int(x + r + 1), ty2 = int(y + r + 1);
        if (tx2 >= 0 && ty2 >=0 && tx1 < int(xgrid) && ty1 < int(ygrid))
        {
            const int px1 = maxv(tx1, 0), py1 = maxv(ty1, 0), px2 = minv(tx2, (int)xgrid), py2 = minv(ty2, (int)ygrid);
            t sum = t();
            for (int i = py1; i < py2; i++)
                for (int ii = px1; ii < px2; ii++)
                    if ((x - ii)*(x - ii) + (y - i)*(y - i) <= rr)
                        sum += grid_array[i * xgrid + ii];
            return sum;
        }
        return t();
    }
    t find_disk_max(const double x, const double y, const double r)
    {
        const double rr = r*r;
        const int tx1 = int(x - r), ty1 = int(y - r), tx2 = int(x + r + 1), ty2 = int(y + r + 1);
        if (tx2 >= 0 && ty2 >=0 && tx1 < int(xgrid) && ty1 < int(ygrid))
        {
            const int px1 = maxv(tx1, 0), py1 = maxv(ty1, 0), px2 = minv(tx2, (int)xgrid), py2 = minv(ty2, (int)ygrid);
            bool found = false;
            t max_check = t();
            for (int i = py1; i < py2; i++)
                for (int ii = px1; ii < px2; ii++)
                    if ((x - ii)*(x - ii) + (y - i)*(y - i) <= rr)
                    {
                        const double val_check = grid_array[i * xgrid + ii];
                         if (!found || val_check > max_check)
                         {
                             max_check = val_check;
                             found = true;
                         }
                    }
            return found ? max_check : t();
        }
        return t();
    }
    t find_disk_min(const double x, const double y, const double r)
    {
        const double rr = r*r;
        const int tx1 = int(x - r), ty1 = int(y - r), tx2 = int(x + r + 1), ty2 = int(y + r + 1);
        if (tx2 >= 0 && ty2 >=0 && tx1 < int(xgrid) && ty1 < int(ygrid))
        {
            const int px1 = maxv(tx1, 0), py1 = maxv(ty1, 0), px2 = minv(tx2, (int)xgrid), py2 = minv(ty2, (int)ygrid);
             bool found = false;
             t min_check = t();
            for (int i = py1; i < py2; i++)
                for (int ii = px1; ii < px2; ii++)
                    if ((x - ii)*(x - ii) + (y - i)*(y - i) <= rr)
                    {
                        const double val_check = grid_array[i * xgrid + ii];
                        if (!found || val_check < min_check)
                        {
                            min_check = val_check;
                            found = true;
                        }
                    }
            return found ? min_check : t();
        }
        return t();
    }
    t find_disk_mean(const double x, const double y, const double r)
    {
        const double rr = r*r;
        const int tx1 = int(x - r), ty1 = int(y - r), tx2 = int(x + r + 1), ty2 = int(y + r + 1);
        if (tx2 >= 0 && ty2 >=0 && tx1 < int(xgrid) && ty1 < int(ygrid))
        {
            const int px1 = maxv(tx1, 0), py1 = maxv(ty1, 0), px2 = minv(tx2, (int)xgrid), py2 = minv(ty2, (int)ygrid);
            t sum = t();
            double region_size = 0;
            for (int i = py1; i < py2; i++)
                for (int ii = px1; ii < px2; ii++)
                    if ((x - ii)*(x - ii) + (y - i)*(y - i) <= rr)
                    {
                        sum += grid_array[i * xgrid + ii];
                        ++region_size;
                    }
           if (region_size == 0)
               return t();

           return sum/region_size;
        }
        return t();
    }
    bool value_region_exists(unsigned int x1, unsigned int y1, unsigned int x2, unsigned int y2, const t val)
    {
       const int tx1 = minv(x1, x2),  ty1 = minv(y1, y2), tx2 = maxv(x1, x2), ty2 = maxv(y1, y2), xd = xgrid - tx1, yd = ygrid - ty1;
       if (xd > 0 && yd > 0)
       {
           const int px1 = maxv(tx1, 0), py1 = maxv(ty1, 0), px2 = minv(tx2 + 1, (int)xgrid), py2 = minv(ty2 + 1, (int)ygrid);
           for (int i = py1; i < py2; i++)
               for (int ii = px1; ii < px2; ii++)
                   if (tequal(grid_array[i * xgrid + ii], val))
                       return true;
       }
       return false;
    }
    int value_region_x(unsigned int x1, unsigned int y1, unsigned int x2, unsigned int y2, const t val)
    {
       const int tx1 = minv(x1, x2),  ty1 = minv(y1, y2), tx2 = maxv(x1, x2), ty2 = maxv(y1, y2), xd = xgrid - tx1, yd = ygrid - ty1;
       if (xd > 0 && yd > 0)
       {
           const int px1 = maxv(tx1, 0), py1 = maxv(ty1, 0), px2 = minv(tx2 + 1, (int)xgrid), py2 = minv(ty2 + 1, (int)ygrid);
           for (int i = py1; i < py2; i++)
               for (int ii = px1; ii < px2; ii++)
                   if (tequal(grid_array[i * xgrid + ii], val))
                      return ii;
       }
       return 0;
    }
    int value_region_y(unsigned int x1, unsigned int y1, unsigned int x2, unsigned int y2, const t val)
    {
       const int tx1 = minv(x1, x2),  ty1 = minv(y1, y2), tx2 = maxv(x1, x2), ty2 = maxv(y1, y2), xd = xgrid - tx1, yd = ygrid - ty1;
       if (xd > 0 && yd > 0)
       {
           const int px1 = maxv(tx1, 0), py1 = maxv(ty1, 0), px2 = minv(tx2 + 1, (int)xgrid), py2 = minv(ty2 + 1, (int)ygrid);
           for (int i = py1; i < py2; i++)
               for (int ii = px1; ii < px2; ii++)
                   if (tequal(grid_array[i * xgrid + ii], val))
                       return i;
       }
       return 0;
    }
    bool value_disk_exists(const double x, const double y, const double r, const t val)
    {
        const double rr = r*r;
        const int tx1 = int(x - r), ty1 = int(y - r), tx2 = int(x + r + 1), ty2 = int(y + r + 1);
        if (tx2 >= 0 && ty2 >=0 && tx1 < int(xgrid) && ty1 < int(ygrid))
        {
            const int px1 = maxv(tx1, 0), py1 = maxv(ty1, 0), px2 = minv(tx2, (int)xgrid), py2 = minv(ty2, (int)ygrid);
            for (int i = py1; i < py2; i++)
                for (int ii = px1; ii < px2; ii++)
                    if ((x - ii)*(x - ii) + (y - i)*(y - i) <= rr)
                        if (tequal(grid_array[i * xgrid + ii], val))
                            return true;
        }
        return false;
    }
    int value_disk_x(const double x, const double y, const double r, const t val)
    {
        const double rr = r*r;
        const int tx1 = int(x - r), ty1 = int(y - r), tx2 = int(x + r + 1), ty2 = int(y + r + 1);
        if (tx2 >= 0 && ty2 >=0 && tx1 < int(xgrid) && ty1 < int(ygrid))
        {
            const int px1 = maxv(tx1, 0), py1 = maxv(ty1, 0), px2 = minv(tx2, (int)xgrid), py2 = minv(ty2, (int)ygrid);
            for (int i = py1; i < py2; i++)
                for (int ii = px1; ii < px2; ii++)
                    if ((x - ii)*(x - ii) + (y - i)*(y - i) <= rr)
                        if (tequal(grid_array[i * xgrid + ii], val))
                            return ii;
        }
        return 0;
    }
    int value_disk_y(const double x, const double y, const double r, const t val)
    {
        const double rr = r*r;
        const int tx1 = int(x - r), ty1 = int(y - r), tx2 = int(x + r + 1), ty2 = int(y + r + 1);
        if (tx2 >= 0 && ty2 >=0 && tx1 < int(xgrid) && ty1 < int(ygrid))
        {
            const int px1 = maxv(tx1, 0), py1 = maxv(ty1, 0), px2 = minv(tx2, (int)xgrid), py2 = minv(ty2, (int)ygrid);
            for (int i = py1; i < py2; i++)
                for (int ii = px1; ii < px2; ii++)
                    if ((x - ii)*(x - ii) + (y - i)*(y - i) <= rr)
                        if (tequal(grid_array[i * xgrid + ii], val))
                            return i;
        }
        return 0;
    }
    void shuffle()
    {
        if (xgrid == 0 || ygrid == 0)
            return;

        const size_t count = static_cast<size_t>(xgrid) * ygrid;
        mt_random_shuffle(grid_array, grid_array + count);
    }
};

/* ds_grids */

static map<unsigned int, grid<variant> > ds_grids;
static unsigned int ds_grids_maxid = 0;

namespace enigma_user
{

unsigned int ds_grid_create(const unsigned int w, const unsigned int h)
{
  //Creates a new grid. The function returns an integer as an id that must be used in all other functions to access the particular grid.
  pair<map<unsigned int, grid<variant> >::iterator, bool> ins = ds_grids.insert(pair<unsigned int, grid<variant> >(ds_grids_maxid++, grid<variant>(w, h)));
  ins.first->second.clear(0);
  return ds_grids_maxid-1;
}

void ds_grid_destroy(const unsigned int id)
{
  //Destroys the grid
  map<unsigned int, grid<variant> >::iterator it = ds_grids.find(id);
  if (it != ds_grids.end())
  {
    it->second.destroy();
    ds_grids.erase(it);
  }
}

void ds_grid_clear(const unsigned int id, const variant val)
{
  //Clears the grid with the given id, to the indicated value
  ds_grids[id].clear(val);
}

void ds_grid_copy(const unsigned int id, const unsigned int source)
{
  //Copies the source grid onto the grid
  ds_grids[id].copy(ds_grids[source]);
}

void ds_grid_resize(const unsigned int id, const unsigned int w, const unsigned int h)
{
  ds_grids[id].resize(w, h);
}

unsigned int ds_grid_width(const unsigned int id)
{
  //Returns the width of the grid
  return ds_grids[id].width();
}

unsigned int ds_grid_height(const unsigned int id)
{
  //Returns the height of the grid
  return ds_grids[id].height();
}

void ds_grid_set(const unsigned int id, const unsigned int x, const unsigned int y, const variant val)
{
  //Sets the indicated cell in the grid with the given id, to the indicated value
  ds_grids[id].insert(x, y, val);
}

void ds_grid_add(const unsigned int id, const unsigned int x, const unsigned int y, const variant val)
{
  //Add the value to the cell in the region in the grid with the given id. For strings this corresponds to concatenation
  ds_grids[id].add(x, y, val);
}

void ds_grid_multiply(const unsigned int id, const unsigned int x, const unsigned int y, const double val)
{
  //Multiplies the value to the cells in the region in the grid with the given id
  ds_grids[id].multiply(x, y, val);
}

void ds_grid_set_region(const unsigned int id, const unsigned int x1, const unsigned int y1, const unsigned int x2, const unsigned int y2, const variant val)
{
  //Sets the all cells in the region in the grid with the given id, to the indicated value
  ds_grids[id].insert_region(x1, y1, x2, y2, val);
}

void ds_grid_add_region(const unsigned int id, const unsigned int x1, const unsigned int y1, const unsigned int x2, const unsigned int y2, const variant val)
{
  //Add the value to the cell in the region in the grid with the given id.
  ds_grids[id].add_region(x1, y1, x2, y2, val);
}

void ds_grid_multiply_region(const unsigned int id, const unsigned int x1, const unsigned int y1, const unsigned int x2, const unsigned int y2, const double val)
{
  //Multiplies the value to the cells in the region in the grid with the given id. Is only valid for numbers
  ds_grids[id].multiply_region(x1, y1, x2, y2, val);
}

void ds_grid_set_disk(const unsigned int id, const double x, const double y, const double r, const variant val)
{
  //Sets all cells in the disk with center (xm,ym) and radius r
  ds_grids[id].insert_disk(x, y, r, val);
}

void ds_grid_add_disk(const unsigned int id, const double x, const double y, const double r, const variant val)
{
  //Add the value to all cells in the disk with center (xm,ym) and radius r
  ds_grids[id].add_disk(x, y, r, val);
}

void ds_grid_multiply_disk(const unsigned int id, const double x, const double y, const double r, const double val)
{
  //Multiply the value to all cells in the disk with center (xm,ym) and radius r
  ds_grids[id].multiply_disk(x, y, r, val);
}

void ds_grid_set_grid_region(const unsigned int id, const unsigned int source, const unsigned int x1, const unsigned int y1, const unsigned int x2, const unsigned int y2, const unsigned int xpos, const unsigned int ypos)
{
  //Copies the contents of the cells in the region in grid source to grid id. xpos and ypos indicate the place where the region must be placed in the grid
  ds_grids[id].insert_grid_region(ds_grids[source], x1, y1, x2, y2, xpos, ypos);
}

void ds_grid_add_grid_region(const unsigned int id, const unsigned int source, const unsigned int x1, const unsigned int y1, const unsigned int x2, const unsigned int y2, const unsigned int xpos, const unsigned int ypos)
{
  //Adds the contents of the cells in the region in grid source to grid id. xpos and ypos indicate the place where the region must be added in the grid
  ds_grids[id].add_grid_region(ds_grids[source], x1, y1, x2, y2, xpos, ypos);
}

void ds_grid_multiply_grid_region(const unsigned int id, const unsigned int source, const unsigned int x1, const unsigned int y1, const unsigned int x2, const unsigned int y2, const unsigned int xpos, const unsigned int ypos)
{
  //Multiplies the contents of the cells in the region in grid source to grid id. xpos and ypos indicate the place where the region must be multiplied in the grid
  ds_grids[id].multiply_grid_region(ds_grids[source], x1, y1, x2, y2, xpos, ypos);
}

variant ds_grid_get(const unsigned int id, const unsigned int x, const unsigned int y)
{
  //Returns the value of the indicated cell in the grid with the given id
  return ((x < ds_grids[id].width() && y < ds_grids[id].height()) ? ds_grids[id].find(x, y) : variant());
}

variant ds_grid_get_sum(const unsigned int id, const unsigned int x1, const unsigned int y1, const unsigned int x2, const unsigned int y2)
{
  //Returns the sum of the values of the cells in the region in the grid with the given id
  return (((x1 < ds_grids[id].width() || x2 < ds_grids[id].width()) && (y1 < ds_grids[id].height() || y2 < ds_grids[id].height())) ? ds_grids[id].find_region_sum(x1, y1, x2, y2) : variant());
}

variant ds_grid_get_max(const unsigned int id, const unsigned int x1, const unsigned int y1, const unsigned int x2, const unsigned int y2)
{
  //Returns the max of the values of the cells in the region in the grid with the given id
  return (((x1 < ds_grids[id].width() || x2 < ds_grids[id].width()) && (y1 < ds_grids[id].height() || y2 < ds_grids[id].height())) ? ds_grids[id].find_region_max(x1, y1, x2, y2) : variant());
}

variant ds_grid_get_min(const unsigned int id, const unsigned int x1, const unsigned int y1, const unsigned int x2, const unsigned int y2)
{
  //Returns the min of the values of the cells in the region in the grid with the given id
  return (((x1 < ds_grids[id].width() || x2 < ds_grids[id].width()) && (y1 < ds_grids[id].height() || y2 < ds_grids[id].height())) ? ds_grids[id].find_region_min(x1, y1, x2, y2) : variant());
}

variant ds_grid_get_mean(const unsigned int id, const unsigned int x1, const unsigned int y1, const unsigned int x2, const unsigned int y2)
{
  //Returns the mean of the values of the cells in the region in the grid with the given id
  return (((x1 < ds_grids[id].width() || x2 < ds_grids[id].width()) && (y1 < ds_grids[id].height() || y2 < ds_grids[id].height())) ? ds_grids[id].find_region_mean(x1, y1, x2, y2) : variant());
}

variant ds_grid_get_disk_sum(const unsigned int id, const double x, const double y, const double r)
{
  //Returns the sum of the values of the cells in the disk
  return (ds_grids[id].find_disk_sum(x, y, r));
}

variant ds_grid_get_disk_max(const unsigned int id, const double x, const double y, const double r)
{
  //Returns the max of the values of the cells in the disk.
  return (ds_grids[id].find_disk_max(x, y, r));
}

variant ds_grid_get_disk_min(const unsigned int id, const double x, const double y, const double r)
{
  //Returns the min of the values of the cells in the disk.
  return (ds_grids[id].find_disk_min(x, y, r));
}

variant ds_grid_get_disk_mean(const unsigned int id, const double x, const double y, const double r)
{
  //Returns the mean of the values of the cells in the disk
  return (ds_grids[id].find_disk_mean(x, y, r));
}

bool ds_grid_value_exists(const unsigned int id, const unsigned int x1, const unsigned int y1, const unsigned int x2, const unsigned int y2, const variant val)
{
  //Returns whether the value appears somewhere in the region
  return (((x1 < ds_grids[id].width() || x2 < ds_grids[id].width()) && (y1 < ds_grids[id].height() || y2 < ds_grids[id].height())) ? ds_grids[id].value_region_exists(x1, y1, x2, y2, val) : false);
}

int ds_grid_value_x(const unsigned int id, const unsigned int x1, const unsigned int y1, const unsigned int x2, const unsigned int y2, const variant val)
{
  //Returns the x-coordinate of the cell in which the value appears in the region
  return (((x1 < ds_grids[id].width() || x2 < ds_grids[id].width()) && (y1 < ds_grids[id].height() || y2 < ds_grids[id].height())) ? ds_grids[id].value_region_x(x1, y1, x2, y2, val) : 0);
}

int ds_grid_value_y(const unsigned int id, const unsigned int x1, const unsigned int y1, const unsigned int x2, const unsigned int y2, const variant val)
{
  //Returns the y-coordinate of the cell in which the value appears in the region
  return (((x1 < ds_grids[id].width() || x2 < ds_grids[id].width()) && (y1 < ds_grids[id].height() || y2 < ds_grids[id].height())) ? ds_grids[id].value_region_y(x1, y1, x2, y2, val) : 0);
}

bool ds_grid_value_disk_exists(const unsigned int id, const double x, const double y, const double r, const variant val)
{
  //Returns whether the value appears somewhere in the disk
  return (ds_grids[id].value_disk_exists(x, y, r, val));
}

int ds_grid_value_disk_x(const unsigned int id, const double x, const double y, const double r, const variant val)
{
  //Returns the x-coordinate of the cell in which the value appears in the disk
  return (ds_grids[id].value_disk_x(x, y, r, val));
}

int ds_grid_value_disk_y(const unsigned int id, const double x, const double y, const double r, const variant val)
{
  //Returns the y-coordinate of the cell in which the value appears in the disk
  return (ds_grids[id].value_disk_y(x, y, r, val));
}

void ds_grid_shuffle(const unsigned int id)
{
  //Shuffles the values in the grid such that they end up in a random order
  ds_grids[id].shuffle();
}

bool ds_grid_exists(const unsigned int id)
{
  //returns whether the grid exists
  return (ds_grids.find(id) != ds_grids.end());
}

unsigned int ds_grid_duplicate(const unsigned int source)
{
  //creates and returns a new grid containing a copy of the source grid
  const unsigned int id = ds_grids_maxid++;
  ds_grids.insert(pair<unsigned int, grid<variant> >(id, grid<variant>(0, 0)));
  ds_grids[id].copy(ds_grids[source]);
  return id;
}

std::string ds_grid_write(const unsigned int id)
{
  std::stringstream ss;
  ss.flags(std::ios::hex | std::ios::uppercase | std::ios::internal);
  ss.width(4);
  ss.fill('0');

  const grid<variant>& dsGrid = ds_grids[id];

  // Write size
  ss << std::hex << dsGrid.width();
  ss.width(4); ss << std::hex << dsGrid.height();

  //ds_grids[id].find(x, y)

  for (unsigned y = 0; y < dsGrid.height(); ++y)
  {
    for (unsigned x = 0; x < dsGrid.width(); ++x)
    {
      // Write coords
      ss.width(4); ss << x;
      ss.width(4); ss << y;

      variant vari = dsGrid.find(x, y);

      // Write type
      ss.width(2);
      ss << (unsigned int)((vari.type == ty_real) ? 0x00 : 0x01);

      // Write data
      if (vari.type == ty_real)
      {
        ss.width(16);
        char* b = (char*)&vari.rval.d;
        for (unsigned i = 0; i < sizeof(double); ++i)
          ss << b[i];
      }
      else
      {
        ss.width(4); ss << vari.string_length();
        ss.width(1);
        for (size_t j = 0; j < vari.string_length(); ++j)
          ss << vari.char_at(j);
      }
    }
  }

  return ss.str();
}

void ds_grid_read(const unsigned int id, std::string value)
{
  std::stringstream ss;
  size_t i = 8;
  unsigned long width_value;
  unsigned long height_value;

  // Read count
  if (!ds_read_hex(value, 0, 4, width_value))
    return;
  if (!ds_read_hex(value, 4, 4, height_value))
    return;

  const unsigned int width = static_cast<unsigned int>(width_value);
  const unsigned int height = static_cast<unsigned int>(height_value);
  
  if (!ds_read_has_bytes(value, 0, 8))
    return;

  if (height != 0 &&
      width > std::numeric_limits<size_t>::max() / height)
    return;

  const size_t cell_count = static_cast<size_t>(width) * height;
  if (cell_count > (value.size() - 8) / 10)
    return;
  
  for (int y = 0; y < height; ++y)
  {
    for (int x = 0; x < width; ++x)
      if (!ds_read_has_bytes(value, i, 10))
        return;    
    
    {
      int xx, yy, type;

      // Read x/y
      unsigned long xx_value;
      unsigned long yy_value;

      if (!ds_read_hex(value, i, 4, xx_value))
        return;
      
      i += 4;
      
      if (!ds_read_hex(value, i, 4, yy_value))
        return;
      i += 4;
      
      xx = static_cast<int>(xx_value);
      yy = static_cast<int>(yy_value);

      // Read type
      unsigned long type_value;
      if (!ds_read_hex(value, i, 2, type_value))
        return;

      type = static_cast<int>(type_value);
      i += 2;

      if (type == 0)
      {
        variant vari;
        vari.type = ty_real;
        if (!ds_read_has_bytes(value, i, 16))
          return;

        string b;
        ss << std::hex << value.substr(i, 16);
        ss >> b;
        double d = atof(b.c_str());

        ss.clear();
        i += 16;

        vari.rval.d = d;
        ds_grids[id].add(xx, yy, vari);
      }
      else
      {
        variant vari;
        vari.type = ty_string;
         unsigned long len_value;

        // Read length
        if (!ds_read_hex(value, i, 4, len_value))
          return;
        
        i += 4;
        
        if (len_value > value.size() - i)
          return;

        const size_t len = static_cast<size_t>(len_value);        

        vari.sval() = value.substr(i, len);
        i += len;

        ds_grids[id].add(xx, yy, vari);
      }
    }
  }
}

}

/* ds_maps */

static map<unsigned int, multimap<variant, variant> > ds_maps;
static unsigned int ds_maps_maxid = 0;

namespace enigma_user
{

unsigned int ds_map_create()
{
  //Creates a new map. The function returns an integer as an id that must be used in all other functions to access the particular map.
  ds_maps.insert(pair<unsigned int, multimap<variant, variant> >(ds_maps_maxid++, multimap<variant, variant>()));
  return ds_maps_maxid-1;
}

void ds_map_destroy(const unsigned int id)
{
  //Destroys the map
  map<unsigned int, multimap<variant, variant> >::iterator it = ds_maps.find(id);
  if (it != ds_maps.end())
    ds_maps.erase(it);
}

void ds_map_clear(const unsigned int id)
{
  //Clears all values from the map
  ds_maps[id].clear();
}

void ds_map_copy(const unsigned int id, const unsigned int source)
{
  //Copies the source map onto the map
  ds_maps[id] = ds_maps[source];
}

unsigned int ds_map_size(const unsigned int id)
{
  //Returns the size of the map
  return ds_maps[id].size();
}

bool ds_map_empty(const unsigned int id)
{
  //Returns whether the map contains no values
  return ds_maps[id].empty();
}

void ds_map_add(const unsigned int id, const variant key, const variant val)
{
  //Adds the value and corresponding key to the map.
  ds_maps[id].insert(pair<variant, variant>(key, val));
}

void ds_map_replace(const unsigned int id, const variant key, const variant val)
{
  //TODO: Studio made it so this function will add the value if it is not in the map.
  //GM 8.1 does not have this behaviour. This has also been tested.
  //Possible solution is to check compatibility setting once it's fixed for sources.
  //https://github.com/enigma-dev/enigma-dev/issues/1461
  //If this function is changed to behave this way, please fix it in the Asynchronous dialog
  //extension which had to create a special function to replace a value adding it if it does
  //not exist in the global async_load map.

  //Replaces the value corresponding with the key with a new value
  multimap<variant, variant>::iterator it = ds_maps[id].find(key);
  if (it != ds_maps[id].end())
  {
    ds_maps[id].erase(it);
    ds_maps[id].insert(pair<variant, variant>(key, val));
  }
}

//NOTE: Special function, see todo comment above.
void ds_map_overwrite(const unsigned int id, const variant key, const variant val)
{
  //Replaces the value corresponding with the key with a new value, adding it if it was not found in the map.
  multimap<variant, variant>::iterator it = ds_maps[id].find(key);
  if (it != ds_maps[id].end())
  {
    ds_maps[id].erase(it);
  }
  ds_maps[id].insert(pair<variant, variant>(key, val));
}

void ds_map_delete(const unsigned int id, const variant key)
{
  //Deletes the key and the corresponding value from the map
  multimap<variant, variant>::iterator it = ds_maps[id].find(key);
  if (it != ds_maps[id].end())
  {
    ds_maps[id].erase(it);
  }
}

void ds_map_delete(const unsigned int id, const variant first, const variant last)
{
  //Deletes the keys and corresponding values in the range between first and last
  multimap<variant, variant>::iterator itf = ds_maps[id].find(first), itl = ds_maps[id].find(last);
  if (itf != ds_maps[id].end() && itl != ds_maps[id].end())
  {
    ds_maps[id].erase(itf, itl);
  }
}

bool ds_map_exists(const unsigned int id, const variant key)
{
  //returns whether the key exists in the map
  map<unsigned int, multimap<variant, variant> >::const_iterator map_it = ds_maps.find(id);
  return map_it != ds_maps.end() && map_it->second.find(key) != map_it->second.end();
}

variant ds_map_find_value(const unsigned int id, const variant key)
{
  //Returns the value corresponding to the key in the map
  map<unsigned int, multimap<variant, variant> >::const_iterator map_it = ds_maps.find(id);
  if (map_it == ds_maps.end())
    return variant();

  multimap<variant, variant>::const_iterator it = map_it->second.find(key);
  return (it == map_it->second.end()) ? variant() : it->second;
}

variant ds_map_find_previous(const unsigned int id, const variant key)
{
  //Returns the largest key in the map smaller than the indicated key
  map<unsigned int, multimap<variant, variant> >::const_iterator map_it = ds_maps.find(id);
  if (map_it == ds_maps.end())
    return variant();

  multimap<variant, variant>::reverse_iterator rit;
  variant key_check;
  for (rit = map_it->second.rbegin(); rit != map_it->second.rend(); rit++)
  {
    if ((key_check = (*rit).first) < key)
    {
      return key_check;
    }
  }
  return variant();
}

variant ds_map_find_next(const unsigned int id, const variant key)
{
  //Returns the smallest key in the map larger than the indicated key
   map<unsigned int, multimap<variant, variant> >::const_iterator map_it = ds_maps.find(id);
  if (map_it == ds_maps.end())
    return variant()
  multimap<variant, variant>::iterator it;
  variant key_check;
  for (rit = map_it->second.rbegin(); rit != map_it->second.rend(); rit++)
  {
    if ((key_check = (*it).first) > key)
    {
      return key_check;
    }
  }
  return variant();
}

variant ds_map_find_first(const unsigned int id)
{
  //Returns the smallest key in the map
  map<unsigned int, multimap<variant, variant> >::const_iterator map_it = ds_maps.find(id);
  if (map_it == ds_maps.end() || map_it->second.empty())
    return variant();

  return map_it->second.begin()->first;
}

variant ds_map_find_last(const unsigned int id)
{
  //Returns the largest key in the map
  map<unsigned int, multimap<variant, variant> >::const_iterator map_it = ds_maps.find(id);
  if (map_it == ds_maps.end() || map_it->second.empty())
    return variant();

  return map_it->second.rbegin()->first;
}

bool ds_map_exists(const unsigned int id)
{
  //returns whether the map exists
  return (ds_maps.find(id) != ds_maps.end());
}

unsigned int ds_map_duplicate(const unsigned int source)
{
  //creates and returns a new map containing a copy of the source map
  const unsigned int id = ds_maps_maxid++;
  ds_maps.insert(pair<unsigned int, multimap<variant, variant> >(id, multimap<variant, variant>()));
  ds_maps[id] = ds_maps[source];
  return id;
}

std::string ds_map_write(const unsigned int id)
{
  std::stringstream ss;
  ss.flags(std::ios::hex | std::ios::uppercase | std::ios::internal);
  ss.width(4);
  ss.fill('0');

  const std::multimap<variant, variant>& dsMap = ds_maps[id];

  // Write size
  ss << std::hex << dsMap.size();

  std::multimap<variant, variant>::iterator it = dsMap.begin();
  while (it != dsMap.end())
  {
    // Write type
    ss.width(2);
    ss << (unsigned int)(((*it).first.type == ty_real) ? 0x00 : 0x01);

    // Write data
    if ((*it).first.type == ty_real)
    {
      ss.width(16);
            char* b = (char*)&(*it).first.rval.d;
            for (unsigned i = 0; i < sizeof(double); ++i)
            ss << b[i];
    }
    else
    {
      ss.width(4); ss << (*it).first.string_length();
      ss.width(1);
      for (size_t j = 0; j < (*it).first.string_length(); ++j)
        ss << (*it).first.char_at(j);
    }

    // Write type
    ss.width(2);
    ss << (unsigned int)(((*it).second.type == ty_real) ? 0x00 : 0x01);

    // Write data
    if ((*it).second.type == ty_real)
    {
      ss.width(16);
      char* b = (char*)&(*it).second.rval.d;
      for (unsigned i = 0; i < sizeof(double); ++i)
        ss << b[i];    }
    else
    {
      ss.width(4); ss << (*it).second.string_length();
      ss.width(1);
      for (size_t j = 0; j < (*it).second.string_length(); ++j)
        ss << (*it).second.char_at(j);
    }

    ++it;
  }

  return ss.str();
}

void ds_map_read(const unsigned int id, std::string value)
{
  std::stringstream ss;
  size_t i = 4;
  unsigned long count_value;

  // Read count
  if (!ds_read_hex(value, 0, 4, count_value))
    return;

  const unsigned int count = static_cast<unsigned int>(count_value);

  for (int j = 0; j < count; ++j)
  {
    variant variKey, variValue;
    int type;

    // Read type
    unsigned long type_value;
    if (!ds_read_hex(value, i, 2, type_value))
      return;

    type = static_cast<int>(type_value);

    i += 2;

    if (type == 0)
    {
      unsigned long raw;
      if (!ds_read_hex(value, i, 16, raw))
        return;

      i += 16;

      string b = value.substr(i - 16, 16);
      char *end = nullptr;
      const double d = strtod(b.c_str(), &end);
      if (end == b.c_str())
        return;

      variKey.rval.d = d;
      variKey.type = ty_real;
    }
    else
    {
      unsigned long len_value;

      // Read length
      if (!ds_read_hex(value, i, 4, len_value))
        return;

      i += 4;
      
      if (len_value > value.size() - i)
        return;

      const size_t len = static_cast<size_t>(len_value);

      variKey = value.substr(i, len);
      i += len;
    }

    // Read type
    unsigned long type_value;
    if (!ds_read_hex(value, i, 2, type_value))
      return;

    type = static_cast<int>(type_value);
    
    i += 2;

    if (type == 0)
    {
      if (!ds_read_has_bytes(value, i, 16))
        return;	
		
      string b;
      ss << std::hex << value.substr(i, 16);
      ss >> b;
      double d = atof(b.c_str());

      ss.clear();
      i += 16;

      variValue.rval.d = d;
      variValue.type = ty_real;
    }
    else
    {
      unsigned long len_value;

      // Read length
      if (!ds_read_hex(value, i, 4, len_value))
        return;

      i += 4;
      
      if (len_value > value.size() - i)
        return;

      const size_t len = static_cast<size_t>(len_value);

      variValue = value.substr(i, len);
      i += len;
    }

    // Push value
    ds_maps[id].insert(std::pair<variant, variant>(variKey, variValue));
  }
}

}

/* ds_lists */

static map<unsigned int, vector<variant> > ds_lists;
static unsigned int ds_lists_maxid = 0;

namespace enigma_user
{

unsigned int ds_list_create()
{
  //Creates a new list. The function returns an integer as an id that must be used in all other functions to access the particular list.
  ds_lists.insert(pair<unsigned int, vector<variant> >(ds_lists_maxid++, vector<variant>()));
  return ds_lists_maxid-1;
}

void ds_list_destroy(const unsigned int id)
{
  //Destroys the list
  map<unsigned int, vector<variant> >::iterator it = ds_lists.find(id);
  if (it != ds_lists.end())
    ds_lists.erase(it);
}

void ds_list_clear(const unsigned int id)
{
  //Clears all values from the list
  ds_lists[id].clear();
}

void ds_list_copy(const unsigned int id, const unsigned int source)
{
  //Copies the source list onto the list
  ds_lists[id] = ds_lists[source];
}

unsigned int ds_list_size(const unsigned int id)
{
  //Returns the size of the list
  return ds_lists[id].size();
}

bool ds_list_empty(const unsigned int id)
{
  //Returns whether the list contains no values
  return ds_lists[id].empty();
}

void ds_list_add(const unsigned int id, const enigma::varargs &values)
{
  //Adds the values at the end of the list.
  for (int i = 0; i < values.argc; ++i)
    ds_lists[id].push_back(values.get(i));
}

void ds_list_insert(const unsigned int id, const unsigned int pos, const variant val)
{
  //Inserts val and the given pos in the list
  if (pos < 0 || static_cast<size_t>(pos) > ds_lists[id].size())
    return;
    ds_lists[id].insert(ds_lists[id].begin() + pos, val);
}

void ds_list_replace(const unsigned int id, const unsigned int pos, const variant value)
{
  //replaces the value at pos with the val in the list
  if (pos < 0 || static_cast<size_t>(pos) >= ds_lists[id].size())
  {
    ds_lists[id][pos] = value;
  }
}

void ds_list_delete(const unsigned int id, const unsigned int pos)
{
  //deletes the value at the given pos in the list
  if (pos < 0 || static_cast<size_t>(pos) >= ds_lists[id].size())
    return;

    ds_lists[id].erase(ds_lists[id].begin() + pos);
  }
}

void ds_list_delete(const unsigned int id, const unsigned int first, const unsigned int last)
{
  //Deletes the values in the range between first and last
  if (first <= last && first < ds_lists[id].size() && last < ds_lists[id].size())
  {
    ds_lists[id].erase(ds_lists[id].begin() + first, ds_lists[id].begin() + last+1);
  }
}

int ds_list_find_index(const unsigned int id, const variant val)
{
  for (size_t i = 0; i < ds_lists[id].size(); i++)
  {
    if (ds_lists[id][i] == val)
    {
      return i;
    }
  }
  return -1;
}

variant ds_list_find_value(const unsigned int id, const unsigned int pos)
{
  //Returns the value stored at the indicated position in the list
  map<unsigned int, vector<variant> >::const_iterator list_it =
      ds_lists.find(id);

  if (list_it == ds_lists.end() ||
      pos < 0 ||
      static_cast<size_t>(pos) >= list_it->second.size())

     return variant();

  return list_it->second[pos];
}

void ds_list_sort(const unsigned int id, const bool ascend)
{
  //Sorts the values in the list. When ascend is true the values are sorted in ascending order, otherwise in descending order.
  if (ascend)
  {
    sort(ds_lists[id].begin(), ds_lists[id].end());
  }
  else
  {
    sort(ds_lists[id].rbegin(), ds_lists[id].rend());
  }
}

void ds_list_shuffle(const unsigned int id)
{
  //shuffles the values in the list into a random order
  mt_random_shuffle(ds_lists[id].begin(), ds_lists[id].end());
}

bool ds_list_exists(const unsigned int id)
{
  //returns whether the list exists
  return (ds_lists.find(id) != ds_lists.end());
}

unsigned int ds_list_duplicate(const unsigned int source)
{
  //creates and returns a new list containing a copy of the source list
  const unsigned int id = ds_lists_maxid++;
  ds_lists.insert(pair<unsigned int, vector<variant> >(
      id, vector<variant>()));
  ds_lists[id] = ds_lists[source];
  return id;
}

std::string ds_list_write(const unsigned int id)
{
  std::stringstream ss;
  ss.flags(std::ios::hex | std::ios::uppercase | std::ios::internal);
  ss.width(4);
  ss.fill('0');

  const vector<variant>& dsList = ds_lists[id];

  // Write count
  ss << dsList.size();
  for (size_t i = 0; i < dsList.size(); ++i)
  {
    // Write type
    ss.width(2);
    ss << (unsigned int)((dsList[i].type == ty_real) ? 0x00 : 0x01);

    // Write data
    if (dsList[i].type == ty_real)
    {
      ss.width(16);
      char* b = (char*)&dsList[i].rval.d;
      for (unsigned i = 0; i < sizeof(double); ++i)
          ss << b[i];
    }
    else
    {
      ss.width(4); ss << dsList[i].string_length();
      ss.width(1);
      for (size_t j = 0; j < dsList[i].string_length(); ++j)
        ss << dsList[i].char_at(j);
    }
  }

  return ss.str();
}

void ds_list_read(const unsigned int id, std::string value)
{
  std::stringstream ss;
  int i = 4;
  int count;
  const auto has_bytes = [&value](size_t pos, size_t count) {
    return pos <= value.size() && count <= value.size() - pos;
  };

  // Read count
  if (!has_bytes(0, 4))
    return;

  ss << std::hex << value.substr(0, 4);
  ss >> count;
  ss.clear();

  for (int j = 0; j < count; ++j)
  {
    int type;

    // Read type
    ss << std::hex << value.substr(i, 2);
    ss >> type;
    ss.clear();
    i += 2;

    if (type == 0)
    {
      variant vari;
      vari.type = ty_real;

      string b;
      ss << std::hex << value.substr(i, 16);
      ss >> b;
      double d = atof(b.c_str());

      ss.clear();
      i += 16;

      vari.rval.d = d;
      ds_lists[id].push_back(vari);
    }
    else
    {
      variant vari;
      int len;

      // Read length
      ss << std::hex << value.substr(i, 4);
      ss >> len;
      ss.clear();
      i += 4;

      vari = value.substr(i, len);
      i += len;

      ds_lists[id].push_back(vari);
    }
  }
}

}

/* ds_prioritys */

static map<unsigned int, multimap<variant, variant> > ds_prioritys;
static unsigned int ds_prioritys_maxid = 0;

namespace enigma_user
{

unsigned int ds_priority_create()
{
  //Creates a new priority queue. The function returns an integer as an id that must be used in all other functions to access the particular priority queue.
  ds_prioritys.insert(pair<unsigned int, multimap<variant, variant> >(ds_prioritys_maxid++, multimap<variant, variant>()));
  return ds_prioritys_maxid-1;
}

void ds_priority_destroy(const unsigned int id)
{
  //Destroys the priority queue
  map<unsigned int, multimap<variant, variant> >::iterator it = ds_prioritys.find(id);
  if (it != ds_prioritys.end())
    ds_prioritys.erase(it);
}

void ds_priority_clear(const unsigned int id)
{
  //Clears all values from the priority queue
  ds_prioritys[id].clear();
}

void ds_priority_copy(const unsigned int id, const unsigned int source)
{
  //Copies the source priority queue onto the priority queue
  ds_prioritys[id] = ds_prioritys[source];
}

unsigned int ds_priority_size(const unsigned int id)
{
  //Returns the size of the priority queue
  return ds_prioritys[id].size();
}

bool ds_priority_empty(const unsigned int id)
{
  //Returns whether the priority queue contains no values
  return ds_prioritys[id].empty();
}

void ds_priority_add(const unsigned int id, const variant val, const variant prio)
{
  //Adds the value with the given priority to the priority queue
  ds_prioritys[id].insert(pair<variant, variant>(val, prio));
}

void ds_priority_change_priority(const unsigned int id, const variant val, const variant prio)
{
  //Changes the priority of the given value in the priority queue
  map<unsigned int, multimap<variant, variant> >::iterator priority_it =
      ds_prioritys.find(id);
  if (priority_it == ds_prioritys.end())
    return;

  multimap<variant, variant>::iterator it =
      priority_it->second.find(val);
  if (it != priority_it->second.end())
   {

    priority_it->second.erase(it);
    priority_it->second.insert(pair<variant, variant>(val, prio))
  }
}

variant ds_priority_find_priority(const unsigned int id, const variant val)
{
  //Returns the priority of the given value in the priority queue
  map<unsigned int, multimap<variant, variant> >::const_iterator priority_it =
      ds_prioritys.find(id);
  if (priority_it == ds_prioritys.end())
    return variant();

  multimap<variant, variant>::const_iterator it =
      priority_it->second.find(val);
  return (it == priority_it->second.end()) ? variant() : it->second;
}

void ds_priority_delete_value(const unsigned int id, const variant val)
{
  //Deletes the given value (with its priority) from the priority queue
  map<unsigned int, multimap<variant, variant> >::iterator priority_it =
      ds_prioritys.find(id);
  if (priority_it == ds_prioritys.end())
    return;

  multimap<variant, variant>::iterator it =
      priority_it->second.find(val);
  if (it != priority_it->second.end())

   {
    priority_it->second.erase(it);
  }
}

bool ds_priority_value_exists(const unsigned int id, const variant val)
{
  //returns whether the value exists in the priority queue
  map<unsigned int, multimap<variant, variant> >::const_iterator priority_it =
      ds_prioritys.find(id);
  return priority_it != ds_prioritys.end() &&
         priority_it->second.find(val) != priority_it->second.end();
}

variant ds_priority_delete_min(const unsigned int id)
{
  //Returns and deletes the value with the smallest priority from the priority queue
  map<unsigned int, multimap<variant, variant> >::iterator priority_it =
      ds_prioritys.find(id);
  if (priority_it == ds_prioritys.end())
    return variant();

  multimap<variant, variant>::iterator it = priority_it->second.begin(), it_check;
  if (it == priority_it->second.end()) {return 0;}
  it_check = it++;
  while (it != ds_prioritys[id].end())
  {
    if ((*it).second < (*it_check).second) {it_check = it;}
    it++;
  }
  const variant val = (*it_check).first;
  ds_prioritys[id].erase(it_check);
  return val;
}

variant ds_priority_find_min(const unsigned int id)
{
  //Returns the value with the smallest priority but does not delete it from the priority queue
  multimap<variant, variant>::iterator it = ds_prioritys[id].begin(), it_check;
  if (it == ds_prioritys[id].end()) {return variant();}
  it_check = it++;
  while (it != ds_prioritys[id].end())
  {
    if ((*it).second < (*it_check).second) {it_check = it;}
    it++;
  }
  return ((*it_check).first);
}

variant ds_priority_delete_max(const unsigned int id)
{
  //Returns and deletes the value with the largest priority from the priority queue
  multimap<variant, variant>::iterator it = ds_prioritys[id].begin(), it_check;
  if (it == ds_prioritys[id].end()) {return variant();}
  it_check = it++;
  while (it != ds_prioritys[id].end())
  {
    if ((*it).second > (*it_check).second) {it_check = it;}
    it++;
  }
  const variant val = (*it_check).first;
  ds_prioritys[id].erase(it_check);
  return val;
}

variant ds_priority_find_max(const unsigned int id)
{
  //Returns the value with the smallest priority but does not delete it from the priority queue
  multimap<variant, variant>::iterator it = ds_prioritys[id].begin(), it_check;
  if (it == ds_prioritys[id].end()) {return variant();}
  it_check = it++;
  while (it != ds_prioritys[id].end())
  {
    if ((*it).second > (*it_check).second) {it_check = it;}
    it++;
  }
  return ((*it_check).first);
}

bool ds_priority_exists(const unsigned int id)
{
  //returns whether the priority queue exists
  return (ds_prioritys.find(id) != ds_prioritys.end());
}

unsigned int ds_priority_duplicate(const unsigned int source)
{
  //creates and returns a new priority queue containing a copy of the source priority queue
  const unsigned int id = ds_prioritys_maxid++;
  ds_prioritys.insert(pair<unsigned int, multimap<variant, variant> >(id, ds_prioritys[source]));
  return id;;
}

std::string ds_priority_write(const unsigned int id)
{
  std::stringstream ss;
  ss.flags(std::ios::hex | std::ios::uppercase | std::ios::internal);
  ss.width(4);
  ss.fill('0');

  const std::multimap<variant, variant>& dsPriority = ds_prioritys[id];

  // Write size
  ss << std::hex << dsPriority.size();

  std::multimap<variant, variant>::iterator it = dsPriority.begin();
  while (it != dsPriority.end())
  {
    // Write type
    ss.width(2);
    ss << (unsigned int)(((*it).first.type == ty_real) ? 0x00 : 0x01);
    ss.width(16);
    char* b = (char*)&(*it).second.rval.d;
    for (unsigned i = 0; i < sizeof(double); ++i)
        ss << b[i];

    // Write data
    if ((*it).first.type == ty_real)
    {
      ss.width(16);
      char* b = (char*)&(*it).first.rval.d;
      for (unsigned i = 0; i < sizeof(double); ++i)
          ss << b[i];
    }
    else
    {
      ss.width(4); ss << (*it).first.string_length();
      ss.width(1);
      for (size_t j = 0; j < (*it).first.string_length(); ++j)
        ss << (*it).first.char_at(j);
    }

    ++it;
  }

  return ss.str();
}

void ds_priority_read(const unsigned int id, std::string value)
{
  std::stringstream ss;
  size_t i = 4;
  unsigned long count_value;

  // Read count
  if (!ds_read_hex(value, 0, 4, count_value))
    return;

  const unsigned int count = static_cast<unsigned int>(count_value);

  for (int j = 0; j < count; ++j)
  {
    variant vari, prio;
    int type;
    
    if (!ds_read_has_bytes(value, i, 18))
      return;

    prio.type = ty_real;

    // Read type
    unsigned long type_value;
    if (!ds_read_hex(value, i, 2, type_value))
      return;

    type = static_cast<int>(type_value);
    
    i += 2;

    // Read priority
    if (!ds_read_has_bytes(value, i, 16))
      return;
    
    string b;
    ss << std::hex << value.substr(i, 16);
    ss >> b;
    double d = atof(b.c_str());
    ss.clear();
    i += 16;

    prio = d;

    if (type == 0)
    {
      string b;
      ss << std::hex << value.substr(i, 16);
      ss >> b;
      double d = atof(b.c_str());

      ss.clear();
      i += 16;

      vari.rval.d = d;
      vari.type = ty_real;
    }
    else
    {
      unsigned long len_value;

      // Read length
      if (!ds_read_hex(value, i, 4, len_value))
        return;
            
      i += 4;

      if (len_value > value.size() - i)
        return;

      const size_t len = static_cast<size_t>(len_value);
      vari = value.substr(i, len);
      i += len;
    }

    // Push value
    ds_prioritys[id].insert(std::pair<variant, variant>(vari, prio));
  }
}

}

/* ds_queues */

static map<unsigned int, deque<variant> > ds_queues;
static unsigned int ds_queues_maxid = 0;

namespace enigma_user
{

unsigned int ds_queue_create()
{
  //Creates a new queue. The function returns an integer as an id that must be used in all other functions to access the particular queue.
  ds_queues.insert(pair<unsigned int, deque<variant> >(ds_queues_maxid++, deque<variant>()));
  return ds_queues_maxid-1;
}

void ds_queue_destroy(const unsigned int id)
{
  //Destroys the queue
  map<unsigned int, deque<variant> >::iterator it = ds_queues.find(id);
  if (it != ds_queues.end())
    ds_queues.erase(it);
}

void ds_queue_clear(const unsigned int id)
{
  //Clears all values from the queue
  ds_queues[id].clear();
}

void ds_queue_copy(const unsigned int id, const unsigned int source)
{
  //Copies the source queue onto the queue
  ds_queues[id] = ds_queues[source];
}

unsigned int ds_queue_size(const unsigned int id)
{
  //Returns the size of the queue
  return ds_queues[id].size();
}

bool ds_queue_empty(const unsigned int id)
{
  //Returns whether the queue contains no values
  return ds_queues[id].empty();
}

void ds_queue_enqueue(const unsigned int id, const enigma::varargs &values)
{
  //Adds the values to the back of the queue
  for (int i = 0; i < values.argc; ++i)
    ds_queues[id].push_back(values.get(i));
}

variant ds_queue_dequeue(const unsigned int id)
{
  //Returns the value at the front of the queue and removes it from the queue
  deque<variant>::iterator it = ds_queues[id].begin();
  if (it == ds_queues[id].end()) {return 0;}
  const variant val = *it;
  ds_queues[id].pop_front();
  return val;
}

variant ds_queue_head(const unsigned int id)
{
  //Returns the value at the front of the queue
  deque<variant>::iterator it = ds_queues[id].begin();
  return ((it == ds_queues[id].end()) ? variant() : (*it));
}

variant ds_queue_tail(const unsigned int id)
{
  //Returns the value on the back of the queue
  deque<variant>::reverse_iterator rit = ds_queues[id].rbegin();
  return ((rit == ds_queues[id].rend()) ? variant() : (*rit));
}

bool ds_queue_exists(const unsigned int id)
{
  //returns whether the queue exists
  return (ds_queues.find(id) != ds_queues.end());
}

unsigned int ds_queue_duplicate(const unsigned int source)
{
  //creates and returns a new queue containing a copy of the source queue
  const unsigned int id = ds_queues_maxid++;
  ds_queues.insert(pair<unsigned int, deque<variant> >(id, deque<variant>()));
  ds_queues[id] = ds_queues[source];
  return id;
}

std::string ds_queue_write(const unsigned int id)
{
  std::stringstream ss;
  ss.flags(std::ios::hex | std::ios::uppercase | std::ios::internal);
  ss.width(4);
  ss.fill('0');

  std::deque<variant> dsQueue = ds_queues[id];

  // Write size
  ss << std::hex << dsQueue.size();

  for (size_t i = 0; i < dsQueue.size(); ++i)
  {
    // Write type
    ss.width(2);
    ss << (unsigned int)((dsQueue[i].type == ty_real) ? 0x00 : 0x01);

    // Write data
    if (dsQueue[i].type == ty_real)
    {
      ss.width(16);
      char* b = (char*)&dsQueue[i].rval.d;
      for (unsigned i = 0; i < sizeof(double); ++i)
          ss << b[i];
    }
    else
    {
      ss.width(4); ss << dsQueue[i].string_length();
      ss.width(1);
      for (size_t j = 0; j < dsQueue[i].string_length(); ++j)
        ss << dsQueue[i].char_at(j);
    }
  }

  return ss.str();
}

void ds_queue_read(const unsigned int id, std::string value)
{
  std::stringstream ss;
  int i = 4;
  int count;

  // Read count
  ss << std::hex << value.substr(0, 4);
  ss >> count;
  ss.clear();

  for (int j = 0; j < count; ++j)
  {
    variant vari;
    int type;

    // Read type
    ss << std::hex << value.substr(i, 2);
    ss >> type;
    ss.clear();
    i += 2;

    if (type == 0)
    {
      string b;
      ss << std::hex << value.substr(i, 16);
      ss >> b;
      double d = atof(b.c_str());

      ss.clear();
      i += 16;

      vari.rval.d = d;
      vari.type = ty_real;
    }
    else
    {
      int len;

      // Read length
      ss << std::hex << value.substr(i, 4);
      ss >> len;
      ss.clear();
      i += 4;

      vari = value.substr(i, len);
      i += len;
    }

    // Push value
    ds_queues[id].push_back(vari);
  }
}

}

/* ds_stacks */

static map<unsigned int, deque<variant> > ds_stacks;
static unsigned int ds_stacks_maxid = 0;

namespace enigma_user
{

unsigned int ds_stack_create()
{
  //Creates a new stack. The function returns an integer as an id that must be used in all other functions to access the particular stack.
  ds_stacks.insert(pair<unsigned int, deque<variant> >(ds_stacks_maxid++, deque<variant>()));
  return ds_stacks_maxid-1;
}

void ds_stack_destroy(const unsigned int id)
{
  //Destroys the stack
  map<unsigned int, deque<variant> >::iterator it = ds_stacks.find(id);
  if (it != ds_stacks.end())
    ds_stacks.erase(it);
}

void ds_stack_clear(const unsigned int id)
{
  //Clears all values from the stack
  ds_stacks[id].clear();
}

void ds_stack_copy(const unsigned int id, const unsigned int source)
{
  //Copies the source stack onto the stack
  ds_stacks[id] = ds_stacks[source];
}

unsigned int ds_stack_size(const unsigned int id)
{
  //Returns the size of the stack
  return ds_stacks[id].size();
}

bool ds_stack_empty(const unsigned int id)
{
  //Returns whether the stack contains no values
  return ds_stacks[id].empty();
}

void ds_stack_push(const unsigned int id, const enigma::varargs &values)
{
  //Pushes the values onto the stack
  for (int i = 0; i < values.argc; ++i)
    ds_stacks[id].push_front(values.get(i));
}

variant ds_stack_pop(const unsigned int id)
{
  //Returns the value on the top of the stack and removes it from the stack
  deque<variant>::iterator it = ds_stacks[id].begin();
  if (it == ds_stacks[id].end()) {return 0;}
  const variant val = *it;
  ds_stacks[id].pop_front();
  return val;
}

variant ds_stack_top(const unsigned int id)
{
  //Returns the value on the top of the stack
  deque<variant>::iterator it = ds_stacks[id].begin();
  return ((it == ds_stacks[id].end()) ? variant() : (*it));
}

bool ds_stack_exists(const unsigned int id)
{
  //returns whether the stack exists
  return (ds_stacks.find(id) != ds_stacks.end());
}

unsigned int ds_stack_duplicate(const unsigned int source)
{
  //creates and returns a new stack containing a copy of the source stack
  const unsigned int id = ds_stacks_maxid++;
  ds_stacks.insert(pair<unsigned int, deque<variant> >(id, deque<variant>()));
  ds_stacks[id] = ds_stacks[source];
  return id;
}

std::string ds_stack_write(const unsigned int id)
{
  std::stringstream ss;
  ss.flags(std::ios::hex | std::ios::uppercase | std::ios::internal);
  ss.width(4);
  ss.fill('0');

  std::deque<variant> dsStack = ds_stacks[id];

  // Write size
  ss << std::hex << dsStack.size();

  for (size_t i = 0; i < dsStack.size(); ++i)
  {
    // Write type
    ss.width(2);
    ss << (unsigned int)((dsStack[i].type == ty_real) ? 0x00 : 0x01);

    // Write data
    if (dsStack[i].type == ty_real)
    {
      ss.width(16);
      char* b = (char*)&dsStack[i].rval.d;
      for (unsigned i = 0; i < sizeof(double); ++i)
        ss << b[i];
    }
    else
    {
      ss.width(4); ss << dsStack[i].string_length();
      ss.width(1);
      for (size_t j = 0; j < dsStack[i].string_length(); ++j)
        ss << dsStack[i].char_at(j);
    }
  }

  return ss.str();
}

void ds_stack_read(const unsigned int id, std::string value)
{
  std::stringstream ss;
  int i = 4;
  int count;

  // Read count
  ss << std::hex << value.substr(0, 4);
  ss >> count;
  ss.clear();

  for (int j = 0; j < count; ++j)
  {
    variant vari;
    int type;

    // Read type
    ss << std::hex << value.substr(i, 2);
    ss >> type;
    ss.clear();
    i += 2;

    if (type == 0)
    {
      string b;
      ss << std::hex << value.substr(i, 16);
      ss >> b;
      double d = atof(b.c_str());

      ss.clear();
      i += 16;

      vari.rval.d = d;
      vari.type = ty_real;
    }
    else
    {
      int len;

      // Read length
      ss << std::hex << value.substr(i, 4);
      ss >> len;
      ss.clear();
      i += 4;

      vari = value.substr(i, len);
      i += len;
    }

    // Push value
    ds_stacks[id].push_back(vari);
  }
}

}

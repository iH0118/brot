__kernel void etime_px(__global unsigned int* dat, double pos_x, double pos_y, unsigned int depth)
{
    int id = get_global_id(0);
    __private double x, y, x2, y2;
    __private unsigned int val = 0;

    while (x2 + y2 <= 4 && val < depth) {
        y = (x + x) * y + y0;
        x = x2 - y2 + x0;

        x2 = x * x;
        y2 = y * y;

        ++val;
    }

    dat[id] = val;
}

// TODO: is redefinition needed here?
struct star
{
    int catalog_number;
    float magnitude;
    double right_ascension;
    double declination;
    double altitude;
    double azimuth;
};

int star_magnitude_comparator(const void *v1, const void *v2)
{
    const struct star *p1 = (struct star *)v1;
    const struct star *p2 = (struct star *)v2;

    // lower magnitudes are brighter
    if (p1->magnitude < p2->magnitude)
        return +1;
    else if (p1->magnitude > p2->magnitude)
        return -1;
    else
        return 0;
}
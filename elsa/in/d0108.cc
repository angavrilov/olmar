// error: duplicate member declaration of `ip_opts' in struct ip_opts; previous at

// this was wrapped inside two 'extern "C"'-s actually, but the error
// is reproducible without them so I took them off.

struct ip_opts
{
  char ip_opts[40];
};

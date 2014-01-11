#include	<sys/types.h>
#include	<dirent.h>
#include	<sys/stat.h>
#include	<fcntl.h>
#include	<stdio.h>
#include	<stdlib.h>
#include	<unistd.h>
#include	<string.h>

#define	SYS_PS "/sys/class/power_supply/"
#define BATTSTAT_BUFFER_SIZE 256

static char bs_buff[BATTSTAT_BUFFER_SIZE];

struct remtime
{
  int	h;
  int	m;
};

#define BATTSTAT_CHARGING	1
#define BATTSTAT_DISCHARGING	2
#define BATTSTAT_CHARGED	3
struct battstat
{
  int			fd;
  char			name[256];
  int			present;
  int			status;
  int			percent;
  struct remtime	time;
  int			remtime;

  int			power_now;
  int			charge_now;
  int			charge_full;
};

char**
get_battlist() {
  
  struct dirent	*file;
  DIR		*rep;
  char		**battlist;
  char		*dest;
  int		nb_batt;
  int		count_char;

  nb_batt = 0;
  count_char = 0;
  rep = opendir(SYS_PS);
  while ((file = readdir(rep))) {

    if (file->d_name[0] == '.' && 
	(file->d_name[1] == '.' || file->d_name[1] == '\0'))
	continue;
    count_char += strlen(file->d_name);
    ++nb_batt;
  }
  
  battlist = malloc((sizeof(char *) * (nb_batt + 1)) + (sizeof(char) * (count_char + nb_batt)));
  dest = (char *)(battlist + nb_batt + 1);
  nb_batt = 0;

  rewinddir(rep);
  while ((file = readdir(rep))) {

    if (file->d_name[0] == '.' && 
	(file->d_name[1] == '.' || file->d_name[1] == '\0'))
	continue;

    battlist[nb_batt] = dest;
    dest = stpcpy(battlist[nb_batt], file->d_name);
    ++dest;
    ++nb_batt;
  }

  closedir(rep);
  battlist[nb_batt] = NULL;
  return (battlist);
}

int
is_present(const char *battname)
{
  char		*p;
  struct stat	st;

  if (snprintf(bs_buff, sizeof(bs_buff), "%s/%s/present", SYS_PS, battname) >= sizeof(bs_buff))
    {
      fprintf(stderr, "Error: bs_buff size to small (change #define BATTSTAT_BUFFER_SIZE bigger than %d\n", BATTSTAT_BUFFER_SIZE);
      exit(1);
    }

  if (stat(bs_buff, &st) == -1)
    return (0);

  return (1);
}

int
close_battinfo(struct battstat *batt)
{
  if (batt->present)
    close(batt->fd);
  return (0);
}

static void
parse_infoline(struct battstat *batt, const char *key, const char *value)
{
  if (!strcmp("POWER_SUPPLY_CURRENT_NOW", key) || 
      !strcmp("POWER_SUPPLY_POWER_NOW", key))
    batt->power_now = atoi(value);

  else if (!strcmp("POWER_SUPPLY_ENERGY_NOW", key) ||
	   !strcmp("POWER_SUPPLY_CHARGE_NOW", key))
    batt->charge_now = atoi(value);

  else if (!strcmp("POWER_SUPPLY_ENERGY_FULL", key) ||
	   !strcmp("POWER_SUPPLY_CHARGE_FULL", key))
    batt->charge_full = atoi(value);

  else if (!strcmp("POWER_SUPPLY_STATUS", key))
    {
      if (!strcmp("Discharging", value))
	batt->status = BATTSTAT_DISCHARGING;
      else if (!strcmp("Charging", value))
	batt->status = BATTSTAT_CHARGING;
      else
	batt->status = BATTSTAT_CHARGED;
    }
}

int
reload_battinfo(struct battstat *batt)
{
  int		nb_r;
  char		*p;
  int		ibuff;
  int		lenline;

  lseek(batt->fd, 0, SEEK_SET);
  ibuff = 0;
  while ((nb_r = read(batt->fd, bs_buff + ibuff, sizeof(bs_buff) - ibuff)) > 0)
    {

      ibuff = 0;
      while ((p = strchr(bs_buff + ibuff, '\n')) != NULL)
	{
	  lenline = p - (bs_buff + ibuff) + 1;
	  *p = '\0';
	  if ((p = strchr(bs_buff + ibuff, '=')) == NULL)
	    {
	      fprintf(stderr, "Bad line syntax...\n");
	      return (-1);
	    }
	  *p = '\0';
	  
	  parse_infoline(batt, bs_buff + ibuff, p + 1);
	  
	  ibuff += lenline;
	}

      memmove(bs_buff, bs_buff + ibuff, sizeof(bs_buff) - ibuff);
      ibuff = sizeof(bs_buff) - ibuff;
    }

  if (batt->status == BATTSTAT_CHARGING)
    batt->remtime = (int)((float)(batt->charge_full - batt->charge_now) / (float)batt->power_now * (float)3600);
  else if (batt->status == BATTSTAT_DISCHARGING)
    batt->remtime = (int)((float)batt->charge_now / (float)batt->power_now * (float)3600);

  batt->percent = (int)((float)batt->charge_now / (float)batt->charge_full * (float)100);

  return (0);
}

int
batt_exist(const char *battname)
{
  char		*p;
  struct stat	st;

  if (snprintf(bs_buff, sizeof(bs_buff), "%s/%s", SYS_PS, battname) >= sizeof(bs_buff))
    {
      fprintf(stderr, "Error: bs_buff size to small (change #define BATTSTAT_BUFFER_SIZE bigger than %d\n", BATTSTAT_BUFFER_SIZE);
      exit(1);
    }

  if (stat(bs_buff, &st) == -1)
    return (0);

  return (1);
}

struct battstat*
open_battinfo(const char *battname)
{
  struct battstat	*bi;
  char			*p;

  if (!batt_exist(battname))
    return (NULL);

  bi = malloc(sizeof(struct battstat));
  strcpy(bi->name, battname);

  if (is_present(battname))
    {
      bi->present = 1;
      p = bs_buff;
      p = stpcpy(p, SYS_PS);
      p = stpcpy(p, battname); 
      p = stpcpy(p, "/uevent");

      bi->fd = open(bs_buff, O_RDONLY);
      reload_battinfo(bi);
    }
  else
    bi->present = 0;

  return (bi);
}

static void
display_batt(struct battstat *bs)
{
  printf("ID: %s\n", bs->name);
  if (bs->present == 0)
    printf("Present: No\n");
  else
    {
      printf("Present: Yes\n");
      
      printf("Charge: %d%%\n", bs->percent);
      if (bs->status == BATTSTAT_DISCHARGING || bs->status == BATTSTAT_CHARGING)
	{
	  bs->time.h = bs->remtime / 3600;
	  bs->time.m = (bs->remtime / 60) % 60;
	  printf("Remaining time: %.2d:%.2d\n", bs->time.h, bs->time.m);
	}
      
      if (bs->status == BATTSTAT_CHARGING)
	printf("Status: Charging\n");
      else if (bs->status == BATTSTAT_DISCHARGING)
	printf("Status: Discharging\n");
      else
	printf("Status: Charged\n");
    }
}

int		main(int ac, char **av)
{
  char		**batt;
  struct battstat *bs;

  if (ac == 1)
    {
      batt = get_battlist();
      while (*batt != NULL)
	{
	  bs = open_battinfo(*batt);
	  display_batt(bs);
	  close_battinfo(bs);
	  
	  ++batt;
	  if (*batt != NULL)
	    printf("\n");	
	}
    }
  else if (ac == 2)
    {
      if ((bs = open_battinfo(av[1])) == NULL)
	{
	  fprintf(stderr, "Error: Battery \"%s\" doesn't exist...\n", av[1]);
	  return (1);
	}
      display_batt(bs);
      close_battinfo(bs);
    }

  return (0);
}

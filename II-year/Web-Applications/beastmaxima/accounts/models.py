from django.db import models
from django.contrib.auth.models import User
from django.db.models.signals import post_save
from django.dispatch import receiver

class PlayerProfile(models.Model):
  email = models.EmailField(unique=True, null=True, blank=True, verbose_name="Email Address")
  user = models.OneToOneField(User, on_delete=models.CASCADE, related_name='profile')
  games_played = models.IntegerField(default=0, verbose_name="Games Played")
  games_won = models.IntegerField(default=0, verbose_name="Games Won")
  highest_score = models.IntegerField(default=0, verbose_name="Highest Score")

  def __str__(self):
    return f"{self.user.username}'s Profile"

@receiver(post_save, sender=User)
def create_user_profile(sender, instance, created, **kwargs):
  if created:
    PlayerProfile.objects.create(user=instance, email=instance.email)

@receiver(post_save, sender=User)
def save_user_profile(sender, instance, **kwargs):
  instance.profile.save()



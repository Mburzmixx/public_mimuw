from django.db import models
from django.contrib.auth.models import User

class Puzzle(models.Model):
  size = models.IntegerField(default=5, verbose_name="Size")
  difficulty = models.CharField(max_length=255, default="normal", verbose_name="Difficulty Level")
  id = models.CharField(max_length=255, primary_key=True, verbose_name="Puzzle ID")
  puzzle_data = models.JSONField(verbose_name="Puzzle Data")
  solution_data = models.JSONField(verbose_name="Solution Data", blank=True, null=True)

  def __str__(self):
    return f"puzzle_{self.id}"
  
class GameSession(models.Model):
  user = models.ForeignKey(User, on_delete=models.CASCADE, related_name='game_sessions', verbose_name="User")
  id = models.AutoField(primary_key=True, verbose_name="Session ID")
  puzzle = models.ForeignKey(Puzzle, on_delete=models.CASCADE, verbose_name="Puzzle")

  start_time = models.DateTimeField(auto_now_add=True, verbose_name="Start Time")
  end_time = models.DateTimeField(null=True, blank=True, verbose_name="End Time")
  time_taken = models.DurationField(null=True, blank=True, verbose_name="Time Taken")
  is_completed = models.BooleanField(default=False, verbose_name="Is Completed")
  
  @property
  def formatted_time_taken(self):
    if self.time_taken:
      return str(self.time_taken).split(".")[0]
    return "--:--:--"

  def __str__(self):
    return f"GameSession for {self.user.username} on {self.puzzle.id}"

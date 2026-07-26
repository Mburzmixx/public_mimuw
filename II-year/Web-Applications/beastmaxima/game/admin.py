from django.contrib import admin
from .models import Puzzle, GameSession

# mateotokoks

@admin.register(Puzzle)
class PuzzleAdmin(admin.ModelAdmin):
  list_display = ('id', 'size', 'difficulty')
  search_fields = ('id', 'difficulty')

@admin.register(GameSession)
class GameSessionAdmin(admin.ModelAdmin):
  list_display = ('user', 'puzzle', 'start_time', 'end_time', 'time_taken', 'is_completed')
  search_fields = ('user__username', 'puzzle__id')
  list_filter = ('is_completed', 'start_time', 'end_time', 'time_taken', 'puzzle__size')


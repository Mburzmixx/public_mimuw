from .models import GameSession

def global_leaderboard(request):
  top_sessions = GameSession.objects.filter(
    is_completed=True
  ).order_by('-puzzle__size', 'time_taken')[:5]
  return {'global_leaderboard': top_sessions}


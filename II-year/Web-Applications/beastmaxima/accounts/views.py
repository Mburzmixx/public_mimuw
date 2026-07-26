from django.shortcuts import render, redirect
from django.contrib.auth import login
from django.contrib.auth.decorators import login_required
from django.contrib.auth.models import User
from django.shortcuts import get_object_or_404
from .forms import RegisterForm

from game.models import GameSession

@login_required
def profile(request, username=None):
  if username:
    profile_user = get_object_or_404(User, username=username)

  else:
    if not request.user.is_authenticated:
      return redirect('account_login')
    profile_user = request.user

  recent_games = GameSession.objects.filter(user=profile_user, is_completed=True).order_by('-end_time')

  return render(request, 'accounts/profile.html', {'recent_games': recent_games, 'profile_user': profile_user})

from django.urls import path
from . import views
from django.views.generic import RedirectView

urlpatterns = [
  path('', RedirectView.as_view(url='lobby/'), name='game_lobby'),
  path('lobby/', views.lobby, name='game_lobby'),
  path('play/<int:size>/', views.play, name='play'),
  path('api/validate/', views.validate_puzzle, name='validate_puzzle'),
  path('api/ranking/', views.get_updated_ranking, name='ranking'),
  path('api/ranking/size/<int:size>/', views.get_updated_size_ranking, name='size_ranking'),
]

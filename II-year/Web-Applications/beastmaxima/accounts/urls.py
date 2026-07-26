from django.urls import path, include
from . import views

urlpatterns = [
  path('', include('allauth.urls')),
  path('profile/', views.profile, name='profile'),
  path('profile/<str:username>/', views.profile, name='profile_with_username'),
]

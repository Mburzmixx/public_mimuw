from django.shortcuts import render
from django.http import HttpResponse
from django.http import HttpResponseRedirect
from django.urls import reverse
import datetime

def home(request):
    return render(request, 'pages/home.html')


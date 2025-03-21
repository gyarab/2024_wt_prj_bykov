from django.contrib import admin
from .models import SearchUser

class SearchUserAdmin(admin.ModelAdmin):
    list_display = ["username", "userDescription", "recentSearches"]

# Register your models here.
admin.site.register(SearchUser, SearchUserAdmin)
from django.contrib import admin
from main.models import *

class SearchUserAdmin(admin.ModelAdmin):
	list_display = ["username", "userDescription", "recentSearches"]

class KeywordListAdmin(admin.ModelAdmin):
	list_display = ["keyword"]

class AllowedAttributeListAdmin(admin.ModelAdmin):
	list_display = ["name", "value"]

class SearchUserAttributesAdmin(admin.ModelAdmin):
	list_display = ["name", "value"]

class SearchUserKeywordListMiddleTableAdmin(admin.ModelAdmin):
	list_display = ["user", "keywordList"]

class SearchUserAttributesMiddleTableAdmin(admin.ModelAdmin):
	list_display = ["user", "attributes"]

admin.site.register(SearchUser, SearchUserAdmin)
admin.site.register(KeywordList, KeywordListAdmin)
admin.site.register(AllowedAttributeList, AllowedAttributeListAdmin)
admin.site.register(SearchUserAttributes, SearchUserAttributesAdmin)
admin.site.register(SearchUserKeywordListMiddleTable, SearchUserKeywordListMiddleTableAdmin)
admin.site.register(SearchUserAttributesMiddleTable, SearchUserAttributesMiddleTableAdmin)


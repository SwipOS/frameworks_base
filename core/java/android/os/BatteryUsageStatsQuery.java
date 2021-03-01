/*
 * Copyright (C) 2020 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package android.os;

import android.annotation.IntDef;
import android.annotation.NonNull;
import android.util.IntArray;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;

/**
 * Query parameters for the {@link BatteryStatsManager#getBatteryUsageStats()} call.
 *
 * @hide
 */
public final class BatteryUsageStatsQuery implements Parcelable {

    @NonNull
    public static final BatteryUsageStatsQuery DEFAULT =
            new BatteryUsageStatsQuery.Builder().build();

    /**
     * Flags for the {@link BatteryStatsManager#getBatteryUsageStats()} method.
     * @hide
     */
    @IntDef(flag = true, prefix = { "FLAG_BATTERY_USAGE_STATS_" }, value = {
            FLAG_BATTERY_USAGE_STATS_POWER_PROFILE_MODEL,
            FLAG_BATTERY_USAGE_STATS_INCLUDE_HISTORY,
    })
    @Retention(RetentionPolicy.SOURCE)
    public @interface BatteryUsageStatsFlags {}

    /**
     * Indicates that power estimations should be based on the usage time and
     * average power constants provided in the PowerProfile, even if on-device power monitoring
     * is available.
     *
     * @hide
     */
    public static final int FLAG_BATTERY_USAGE_STATS_POWER_PROFILE_MODEL = 1;

    /**
     * Indicates that battery history should be included in the BatteryUsageStats.
     * @hide
     */
    public static final int FLAG_BATTERY_USAGE_STATS_INCLUDE_HISTORY = 2;

    private final int mFlags;
    @NonNull
    private final int[] mUserIds;

    private BatteryUsageStatsQuery(@NonNull Builder builder) {
        mFlags = builder.mFlags;
        mUserIds = builder.mUserIds != null ? builder.mUserIds.toArray()
                : new int[]{UserHandle.USER_ALL};
    }

    @BatteryUsageStatsFlags
    public int getFlags() {
        return mFlags;
    }

    /**
     * Returns an array of users for which the attribution is requested.  It may
     * contain {@link UserHandle#USER_ALL} to indicate that the attribution
     * should be performed for all users. Battery consumed by users <b>not</b> included
     * in this array will be returned in the aggregated form as {@link UserBatteryConsumer}'s.
     */
    @NonNull
    public int[] getUserIds() {
        return mUserIds;
    }

    /**
     * Returns true if the power calculations must be based on the PowerProfile constants,
     * even if measured energy data is available.
     */
    public boolean shouldForceUsePowerProfileModel() {
        return (mFlags & FLAG_BATTERY_USAGE_STATS_POWER_PROFILE_MODEL) != 0;
    }

    private BatteryUsageStatsQuery(Parcel in) {
        mFlags = in.readInt();
        mUserIds = new int[in.readInt()];
        in.readIntArray(mUserIds);
    }

    @Override
    public void writeToParcel(Parcel dest, int flags) {
        dest.writeInt(mFlags);
        dest.writeInt(mUserIds.length);
        dest.writeIntArray(mUserIds);
    }

    @Override
    public int describeContents() {
        return 0;
    }

    @NonNull
    public static final Creator<BatteryUsageStatsQuery> CREATOR =
            new Creator<BatteryUsageStatsQuery>() {
                @Override
                public BatteryUsageStatsQuery createFromParcel(Parcel in) {
                    return new BatteryUsageStatsQuery(in);
                }

                @Override
                public BatteryUsageStatsQuery[] newArray(int size) {
                    return new BatteryUsageStatsQuery[size];
                }
            };

    /**
     * Builder for BatteryUsageStatsQuery.
     */
    public static final class Builder {
        private int mFlags;
        private IntArray mUserIds;

        /**
         * Builds a read-only BatteryUsageStatsQuery object.
         */
        public BatteryUsageStatsQuery build() {
            return new BatteryUsageStatsQuery(this);
        }

        /**
         * Add a user whose battery stats should be included in the battery usage stats.
         * {@link UserHandle#USER_ALL} will be used by default if no users are added explicitly.
         */
        public Builder addUser(@NonNull UserHandle userHandle) {
            if (mUserIds == null) {
                mUserIds = new IntArray(1);
            }
            mUserIds.add(userHandle.getIdentifier());
            return this;
        }

        /**
         * Requests that battery history be included in the BatteryUsageStats.
         */
        public Builder includeBatteryHistory() {
            mFlags |= BatteryUsageStatsQuery.FLAG_BATTERY_USAGE_STATS_INCLUDE_HISTORY;
            return this;
        }

        /**
         * Requests to return modeled battery usage stats only, even if on-device
         * power monitoring data is available.
         *
         * Should only be used for testing and debugging.
         */
        public Builder powerProfileModeledOnly() {
            mFlags |= BatteryUsageStatsQuery.FLAG_BATTERY_USAGE_STATS_POWER_PROFILE_MODEL;
            return this;
        }
    }
}
